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

#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32
#define RING_SIZE 1024

static volatile bool force_quit = false;

// 1. 严格 64 字节缓存行对齐的量化行情结构体（防多核伪共享）
struct __rte_cache_aligned MarketDataSnapshot {
    uint32_t instrument_id;  // 标的代码 (如 600519)
    double last_price;       // 最新价
    uint64_t volume;         // 成交量
    uint64_t tsc_timestamp;  // 行情到达网卡的用户态高精度 TSC 时间戳
};

// 全局无锁环形队列指针
struct rte_ring *market_data_ring = NULL;

static void signal_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        printf("\n\n接收到退出信号 %d，系统正在安全清理网络环境...\n", signum);
        force_quit = true;
    }
}

// 2. 策略计算线程 (业务核心核心)：从无锁队列中以极速死循环轮询行情
static int lcore_strategy(void *arg) {
    unsigned lcore_id = rte_lcore_id();

    printf("[Core %u] 策略交易线程成功拉起，开始死循环盯盘...\n", lcore_id);

    void *msg = NULL;

    while (!force_quit) {
        // 从无锁队列中弹出一个行情指针（零拷贝、无锁原子操作）
        if (rte_ring_dequeue(market_data_ring, &msg) == 0) {
            uint64_t now_tsc = rte_rdtsc();

            // 计算纯软件层面的全链路时延（时钟周期数转纳秒）
            uint64_t diff_tsc = now_tsc - md->tsc_timestamp;
            double latency_ns = (double)diff_tsc * 1000000000 / rte_get_tsc_hz();

            // 打印解析出来的业务行情数据及极致延迟统计
            printf("[Core %u] ⚡行情接收! 标的: %06u | 价格: %.2f | 软件穿透延迟: %.2f ns\n", 
                   lcore_id, md->instrument_id, md->last_price, latency_ns);

            // 释放由收包线程在大页内存池中申请的业务空间，严格禁止频繁调用标准 malloc/free
            rte_free(md);
        }
    }
    return 0;
}

// 3. 收包轮询线程 (IO核心)：死循环死磕网卡，零中断，并进行 UDP 解包
static int lcore_to_rx(void *arg) {
    uint16_t port_id = 0; // 接管的第一个 DPDK 网口
    unsigned lcore_id = rte_lcore_id();
    struct rte_mbuf *bufs[BURST_SIZE];

    printf("[Core %u] IO 收包线程启动，开始纯轮询模式 (PMD) 压榨网卡...\n"

    while (!force_quit) {
        // 纯轮询模式：一口气从网卡硬件 Ring Buffer 拉取最多 32 个原始数据包
        uint16_t nb_rx = rte_eth_rx_burst(port_id, 0, bufs, BURST_SIZE);
        if (nb_rx == 0) continue;

        // 捕获到网络包的瞬间，立刻打上纳秒级 CPU 硬件时钟戳
        uint64_t rx_tsc = rte_rdtsc();

        for (uint16_t i = 0; i < nb_rx; i++) {
            struct rte_mbuf *m = bufs[i];
            
            // 零拷贝解析：利用指针偏移直接跳过以太网头、IP头
            struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(m, struct rte_ether_hdr *);
            if (eth_hdr->ether_type == rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4)) {
                struct rte_ipv4_hdr *ip_hdr = (struct rte_ipv4_hdr *)(eth_hdr + 1);
                
                if (ip_hdr->next_proto_id == IPPROTO_UDP) {
                    struct rte_udp_hdr *udp_hdr = (struct rte_udp_hdr *)(ip_hdr + 1);
                    // 业务载荷指针直接映射
                    char *payload = (char *)(udp_hdr + 1); 

                    // 在大页内存中为自定义行情分配空间（绝不在高频路径使用标准 malloc）
                    struct MarketDataSnapshot *md = (struct MarketDataSnapshot *)rte_malloc("MD", sizeof(struct MarketDataSnapshot), 64);
                    if (md != NULL) {
                        // 假设收到的 UDP 载荷前 4 字节是代码，后 8 字节是双精度浮点数价格
                        md->instrument_id = *(uint32_t*)payload;
                        md->last_price = *(double*)(payload + 4);
                        md->volume = 1000;
                        md->tsc_timestamp = rx_tsc; // 标记包刚到内核旁路网卡的时间

                        // 将封装好的低延迟行情塞入无锁队列，极速分发给策略核心
                        if (rte_ring_enqueue(market_data_ring, md) < 0) {
                            rte_free(md); // 队列满则丢弃
                        }
                    }
                }
            }
            rte_pktmbuf_free(m); // 释放原始数据包缓冲区
        }
    }
}

// 4. 网卡基本硬件特性初始化
// 4. 网卡基本硬件特性初始化
static inline int port_init(uint16_t port, struct rte_mempool *mbuf_pool) {
    struct rte_eth_conf port_conf = {0};
    
    // 核心避坑点：Realtek r8169 属于廉价卡，其硬件只支持 1 个接收队列（Rx Queue）和 1 个发送队列
    const uint16_t rx_rings = 1, tx_rings = 1; 
    uint16_t nb_rxd = 1024;
    uint16_t nb_txd = 1024;
    
    struct rte_eth_dev_info dev_info;
    if (!rte_eth_dev_is_valid_port(port)) return -1;

    // 配置网卡通道
    if (rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf) < 0) return -1;

    // 设置接收队列 0
    if (rte_eth_rx_queue_setup(port, 0, nb_rxd, rte_eth_dev_socket_id(port), NULL, mbuf_pool) < 0) return -1;

    // 设置发送队列 0 (即使只用来收包，很多 PMD 驱动也强制要求配置 Tx 队列才能成功 Start)
    struct rte_eth_txconf txconf;
    rte_eth_dev_info_get(port, &dev_info);
    txconf = dev_info.default_txconf;
    if (rte_eth_tx_queue_setup(port, 0, nb_txd, rte_eth_dev_socket_id(port), &txconf) < 0) return -1;

    // 硬件开机
    if (rte_eth_dev_start(port) < 0) return -1;

    // 开启混杂模式（Promiscuous Mode），无视目标 MAC 地址，收下网线上所有的包
    rte_eth_promiscuous_enable(port);
    return 0;
}

int main(int argc, char *argv[])
    struct rte_mempool *mbuf_pool;

    // 初始化 DPDK 环境抽象层 (EAL)
    int ret = rte_eal_init(argc, argv);
    if (ret < 0) rte_exit(EXIT_FAILURE, "EAL 初始化失败\n");
    argc -= ret; argv += ret;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // 检查核心数量，项目需要至少 2 个绑定的隔离核心
    if (rte_lcore_count() < 2) {
        rte_exit(EXIT_FAILURE, "错误：该量化系统必须指定至少 2 个 CPU 核心 (-l 参数)！\n");
    }

        struct rte_mempool *mbuf_pool;

    // 初始化 DPDK 环境抽象层 (EAL)
    int ret = rte_eal_init(argc, argv);
    if (ret < 0) rte_exit(EXIT_FAILURE, "EAL 初始化失败\n");
    argc -= ret; argv += ret;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // 检查核心数量，项目需要至少 2 个绑定的隔离核心
    if (rte_lcore_count() < 2) {
        rte_exit(EXIT_FAILURE, "错误：该量化系统必须指定至少 2 个 CPU 核心 (-l 参数)！\n");
    }

    // 创建大页内存池
    mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS, MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    if (mbuf_pool == NULL) rte_exit(EXIT_FAILURE, "无法创建 mbuf 大页内存池\n");

    // 初始化接管的 r8169 网口
    if (port_init(0, mbuf_pool) != 0) rte_exit(EXIT_FAILURE, "无法配置并初始化网口 0\n");

    // 创建核心数据通道：多生产者单消费者 (MP_SC) 的用户态无锁环形队列
    market_data_ring = rte_ring_create("MARKET_DATA_RING", RING_SIZE, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
    if (market_data_ring == NULL) rte_exit(EXIT_FAILURE, "无法创建用户态无锁环形队列\n");

    // 异步启动：在另一个核心拉起策略计算核心
    unsigned strategy_core = rte_get_next_lcore(rte_get_main_lcore(), 1, 0);
    rte_eal_remote_launch(lcore_strategy, NULL, strategy_core);

    // 主核心亲自下场跑收包死循环，沦为专业的 IO 收包轮询机
    lcore_io_rx(NULL);

    // 等待异步策略核心安全退场
    rte_eal_mp_wait_lcore();

    // 清理网卡硬件状态
    rte_eth_dev_stop(0);
    printf("DPDK 高频行情加速系统已安全退出。\n");
