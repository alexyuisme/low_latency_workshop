#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_ring.h>
#include <rte_net.h>
#include <rte_malloc.h>

#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32
#define RING_SIZE 1024

static volatile bool force_quit = false;

// 1. 严格 64 字节缓存行对齐的量化行情结构体（防伪共享）
struct __rte_cache_aligned MarketDataSnapshot {
    uint32_t instrument_id;  // 标的代码 (如 600519)
    double last_price;       // 最新价
    uint64_t volume;         // 成交量
    uint64_t tsc_timestamp;  // 行情到达网卡的用户态高精度时间戳
};

// 全局无锁队列指针
struct rte_ring *market_data_ring = NULL;

static void signal_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        printf("\n\nSignal %d received, preparing to exit...\n", signum);
        force_quit = true;
    }
}

// 2. 策略计算线程 (业务核心)：从无锁队列中极速死循环轮询行情
static int lcore_strategy(void *arg) {
    unsigned lcore_id = rte_lcore_id();
    printf("Strategy Thread started on core %u\n", lcore_id);

    void *msg = NULL;
    while (!force_quit) {
        // 从无锁队列中弹出一个行情指针（零拷贝、无锁原子操作）
        if (rte_ring_dequeue(market_data_ring, &msg) == 0) {
            struct MarketDataSnapshot *md = (struct MarketDataSnapshot *)msg;
            uint64_t now_tsc = rte_rdtsc();
            
            // 计算纯软件层面的全链路时延（时钟周期数转纳秒）
            uint64_t diff_tsc = now_tsc - md->tsc_timestamp;
            double latency_ns = (double)diff_tsc * 1000000000 / rte_get_tsc_hz();

            // 打印解析出来的业务行情数据及极致延迟统计
            printf("[Core %u] 行情接收! 代码: %06u | 价格: %.2f | 软件穿透延迟: %.2f ns\n", 
                   lcore_id, md->instrument_id, md->last_price, latency_ns);

            // 释放由收包线程在大页内存池中申请的业务空间
            rte_free(md);
        }
    }
    return 0;
}

// 3. 收包轮询线程 (IO核心)：死循环死磕网卡，零中断，并解析 UDP
static int lcore_io_rx(void *arg) {
    uint16_t port_id = 0; // 接管的第一个 DPDK 网口
    unsigned lcore_id = rte_lcore_id();
    struct rte_mbuf *bufs[BURST_SIZE];

    printf("IO RX Thread started on core %u, polling port %u...\n", lcore_id, port_id);

    while (!force_quit) {
        // 纯轮询模式 (PMD)：一口气从网卡硬件 Ring Buffer 拉取最多 32 个原始数据包
        uint16_t nb_rx = rte_eth_rx_burst(port_id, 0, bufs, BURST_SIZE);
        if (nb_rx == 0) continue;

        // 捕获到网络包的瞬间，立刻打上纳秒级 CPU 硬件时钟戳
        uint16_t i;
        for (i = 0; i < nb_rx; i++) {
            struct rte_mbuf *m = bufs[i];
            
            // 零拷贝解析：利用指针偏移直接跳过以太网、IP、UDP 头
            struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(m, struct rte_ether_hdr *);
            if (eth_hdr->ether_type == rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4)) {
                struct rte_ipv4_hdr *ip_hdr = (struct rte_ipv4_hdr *)(eth_hdr + 1);
                
                if (ip_hdr->next_proto_id == IPPROTO_UDP) {
                    struct rte_udp_hdr *udp_hdr = (struct rte_udp_hdr *)(ip_hdr + 1);
                    // 业务载荷直接映射
                    char *payload = (char *)(udp_hdr + 1); 

                    // 在大页内存中为自定义行情分配空间（绝不在高频路径使用标准 malloc）
                    struct MarketDataSnapshot *md = (struct MarketDataSnapshot *)rte_malloc("MD", sizeof(struct MarketDataSnapshot), 64);
                    if (md != NULL) {
                        // 假设收到的 UDP 载荷前 4 字节是代码，后 8 字节是双精度价格（模拟交易所编码）
                        md->instrument_id = *(uint32_t*)payload;
                        md->last_price = *(double*)(payload + 4);
                        md->volume = 1000;
                        md->tsc_timestamp = rte_rdtsc(); // 标记到达时间

                        // 将封装好的低延迟行情塞入无锁队列，分发给策略线程
                        if (rte_ring_enqueue(market_data_ring, md) < 0) {
                            rte_free(md); // 队列满则丢弃
                        }
                    }
                }
            }
            rte_pktmbuf_free(m); // 释放原始数据包缓冲区
        }
    }
    return 0;
}

// 4. 网卡基本硬件特性初始化
static inline int port_init(uint16_t port, struct rte_mempool *mbuf_pool) {
    struct rte_eth_conf port_conf = {0};
    const uint16_t rx_rings = 1, tx_rings = 0;
    uint16_t nb_rxd = 1024;
    
    if (!rte_eth_dev_is_valid_port(port)) return -1;

    if (rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf) < 0) return -1;

    if (rte_eth_rx_queue_setup(port, 0, nb_rxd, rte_eth_dev_socket_id(port), NULL, mbuf_pool) < 0) return -1;

    if (rte_eth_dev_start(port) < 0) return -1;

    rte_eth_promiscuous_enable(port);
    return 0;
}

int main(int argc, char *argv[]) {
    struct rte_mempool *mbuf_pool;
    unsigned lcore_id;

    // 初始化 DPDK 环境抽象层 (EAL)
    int ret = rte_eal_init(argc, argv);
    if (ret < 0) rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");
    argc -= ret; argv += ret;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // 检查核心数量，项目需要至少 2 个绑定的隔离核心
    if (rte_lcore_count() < 2) {
        rte_exit(EXIT_FAILURE, "Error: This system requires at least 2 lcores.\n");
    }

    // 创建大页内存池
    mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS, MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    if (mbuf_pool == NULL) rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

    // 初始化网口
    if (port_init(0, mbuf_pool) != 0) rte_exit(EXIT_FAILURE, "Cannot init port %" PRIu16 "\n", 0);

    // 创建核心数据通道：多生产者单消费者 (MP_SC) 的用户态无锁环形队列
    market_data_ring = rte_ring_create("MARKET_DATA_RING", RING_SIZE, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
    if (market_data_ring == NULL) rte_exit(EXIT_FAILURE, "Cannot create lock-free ring\n");

    // 启动策略计算核心（非主核心，异步拉起）
    unsigned strategy_core = rte_get_next_lcore(rte_get_main_lcore(), 1, 0);
    rte_eal_remote_launch(lcore_strategy, NULL, strategy_core);

    // 主核心直接亲自下场跑死循环，沦为专业的 IO 收包轮询机
    lcore_io_rx(NULL);

    // 等待异步线程安全退出
    rte_eal_mp_wait_lcore();

    // 清理网卡环境
    rte_eth_dev_stop(0);
    printf("DPDK Market Data System clean exit.\n");

    return 0;
}
