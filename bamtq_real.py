from scapy.all import sniff, TCP, UDP, IP
from collections import deque
import time

# -------------------------------
# CONFIGURATION
# -------------------------------
alpha = 0.3
avg_traffic = 0
packet_count = 0
start_time = time.time()
last_print_time = 0
MIN_RATE = 1.0
tenant_queues = {}
credits = {
    "Emergency": 5,
    "Stream": 3,
    "Sensor": 2,
    "Bulk": 1
}
weights = {
    "Emergency": 5,
    "Stream": 3,
    "Sensor": 2,
    "Bulk": 1
}
# Queues
queues = {
    "Emergency": deque(),
    "Stream": deque(),
    "Sensor": deque(),
    "Bulk": deque()
}

# Deadlines (ms)
deadlines = {
    "Emergency": 60000,   # 60 sec
    "Stream": 120000,     # 2 min
    "Sensor": 300000,     # 5 min
    "Bulk": 600000        # 10 min
}


# -------------------------------
# TRAFFIC CLASSIFICATION
# -------------------------------
def classify_packet(pkt):
     if pkt.haslayer(TCP) or pkt.haslayer(UDP):
          dport = pkt[TCP].dport if pkt.haslayer(TCP) else pkt[UDP].dport

          if dport in [80, 443, 8080]:
              traffic_type = "Stream"
          elif dport in [1883, 5683]:
              traffic_type = "Sensor"
          elif dport in [53]:
              traffic_type = "Emergency"   # DNS priority
          else:
              traffic_type = "Bulk"
     else:
          traffic_type = "Emergency"

     return traffic_type

# -------------------------------
# BURST DETECTION (EWMA)
# -------------------------------
def detect_burst(current_rate):
    global avg_traffic

    # Initialize avg properly (prevents startup false burst)
    if avg_traffic == 0:
         avg_traffic = current_rate
         return False

    # Update EWMA
    avg_traffic = alpha * current_rate + (1 - alpha) * avg_traffic

    # ✅ NEW CONDITION (Solution 1 applied)
    if current_rate > MIN_RATE and current_rate > 2.5 * avg_traffic:
        print("🚨 BURST DETECTED!")
        return True

    return False


# -------------------------------
# EDF SCHEDULING
# -------------------------------
def schedule_packet(burst=False):
    global credits
    current_time = time.time()

    # Skip Bulk during burst
    active_queues = {
        k: v for k, v in queues.items()
        if not (burst and k == "Bulk")
    }

    sorted_queues = sorted(active_queues.items(), key=lambda x: deadlines[x[0]])

    for q_name, q in sorted_queues:

        # Remove expired packets
        while q:
            pkt, pkt_time = q[0]
            if (current_time - pkt_time) * 1000 > deadlines[q_name]:
                q.popleft()
                print(f"❌ Dropped from {q_name}")
            else:
                break

        # Credit-based scheduling (DWRR)
        if q and credits[q_name] > 0:
            pkt, _ = q.popleft()
            credits[q_name] -= 1

            # Reset credits using weights (FIXED)
            if credits[q_name] == 0:
                 credits[q_name] = weights[q_name]

            print(f"📤 Transmitting from {q_name} | Credits: {credits[q_name]} | Burst: {burst}")
            return

    print("⚠️ No packets to transmit")


# -------------------------------
# METRICS
# -------------------------------
def compute_throughput():
    global packet_count, start_time
    elapsed = time.time() - start_time
    if elapsed == 0:
        return 0
    return (packet_count * 1024 * 8) / (elapsed * 1e6)  # Mbps


# -------------------------------
# MAIN PACKET HANDLER
# -------------------------------
def process_packet(pkt):
    global packet_count, last_print_time

    packet_count += 1

    # Extract features
    size = len(pkt)
    timestamp = time.time()
    src = pkt[IP].src if pkt.haslayer(IP) else "Unknown"
    # Tenant tracking
    if src not in tenant_queues:
         tenant_queues[src] = deque()

    tenant_queues[src].append(pkt)
    dst = pkt[IP].dst if pkt.haslayer(IP) else "Unknown"

    # Traffic rate
    elapsed = time.time() - start_time
    rate = packet_count / elapsed if elapsed > 0 else 0

    # Burst detection
    burst = detect_burst(rate)

    # Classification
    traffic_type = classify_packet(pkt)

    # Push to queue
    queues[traffic_type].append((pkt, time.time()))

    # Throughput
    throughput = compute_throughput()

    # ✅ MOVE THIS INSIDE FUNCTION
    if time.time() - last_print_time > 0.5:

        print("\n-------------------------------")
        print(f"📦 Packet Size: {size}")
        print(f"⏱ Timestamp: {timestamp}")
        print(f"🌐 {src} → {dst}")
        print(f"📊 Traffic Rate: {rate:.2f} pkt/sec")
        print(f"🔎 Type: {traffic_type}")

        if burst:
            print("🚨 BURST DETECTED!")

        print("📚 Queue Sizes:")
        for q in queues:
            print(f"{q}: {len(queues[q])}")

        print(f"🚀 Throughput: {throughput:.2f} Mbps")
        if any(len(q) > 0 for q in queues.values()):
            for _ in range(min(5, sum(len(q) for q in queues.values()))):
                 schedule_packet(burst=burst)

        last_print_time = time.time()


# -------------------------------
# START SNIFFING
# -------------------------------
print("🚀 Starting Real-Time Network Traffic Analyzer...\n")

sniff(iface="eth0", prn=lambda pkt: process_packet(pkt), store=False)
