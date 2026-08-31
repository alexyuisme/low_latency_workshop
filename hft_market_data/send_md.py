from scapy.all import Ether, IP, UDP, Raw, sendp
import struct

# 构造 payload：4字节 instrument_id (小端) + 8字节 double price
instrument_id = 600519
price = 1888.88
payload = struct.pack('<I', instrument_id) + struct.pack('<d', price)

# 构造以太网帧
pkt = Ether(dst="ff:ff:ff:ff:ff:ff", src="02:00:00:00:00:01") \
   / IP(src="10.0.0.2", dst="10.0.0.255") \
   / UDP(sport=12345, dport=54321) \
   / Raw(load=payload)

print("Sending 100 packets...")
sendp(pkt, iface="tap_md", count=100, inter=0.05)
print("Done.")
