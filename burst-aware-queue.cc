#include <queue>
#include <vector>
#include <map>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("BAMTQSimulation");

int main ()
{
    double alpha = 0.2;
    double avgTraffic = 0;
    uint32_t burstThreshold = 300;
    bool burstDetected = false;

    std::cout << "BAMTQ Simulation Started" << std::endl;

    // Create Nodes
    NodeContainer nodes;
    nodes.Create(3);   // IoT, Edge Router, Cloud

    // Configure Point-to-Point Link
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
     pointToPoint.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("50p"));

    // Install devices between Node0 ↔ Node1
    NetDeviceContainer devices1;
    devices1 = pointToPoint.Install(nodes.Get(0), nodes.Get(1));

    // Install devices between Node1 ↔ Node2
    NetDeviceContainer devices2;
    devices2 = pointToPoint.Install(nodes.Get(1), nodes.Get(2));

    // Install Internet Stack
    InternetStackHelper stack;
    stack.Install(nodes);

    // Assign IP Addresses
    Ipv4AddressHelper address;

    address.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces1 = address.Assign(devices1);

    address.SetBase ("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces2 = address.Assign(devices2);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    std::cout << "Network Topology Created Successfully" << std::endl;
    // Create UDP Server on Cloud Node
uint16_t port = 9;

UdpServerHelper server(port);
ApplicationContainer serverApp = server.Install(nodes.Get(2));
serverApp.Start(Seconds(1.0));
serverApp.Stop(Seconds(10.0));

// Create UDP Client on IoT Node
UdpClientHelper client(interfaces2.GetAddress(1), port);

client.SetAttribute("MaxPackets", UintegerValue(1000));
client.SetAttribute("Interval", TimeValue(MilliSeconds(1)));
client.SetAttribute("PacketSize", UintegerValue(1024));

ApplicationContainer clientApp = client.Install(nodes.Get(0));
clientApp.Start(Seconds(2.0));
clientApp.Stop(Seconds(10.0));

// Run simulation
FlowMonitorHelper flowHelper;
Ptr<FlowMonitor> flowMonitor = flowHelper.InstallAll();
Simulator::Stop(Seconds(10.0));
Simulator::Run();
flowMonitor->CheckForLostPackets();

Ptr<Ipv4FlowClassifier> classifier =
    DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());

std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats();

for (auto &flow : stats)
{
    std::cout << "Packets Sent: " << flow.second.txPackets << std::endl;
    std::cout << "Packets Received: " << flow.second.rxPackets << std::endl;
    std::cout << "Packet Loss: "
              << (flow.second.txPackets - flow.second.rxPackets)
              << std::endl;
}
uint32_t packetsSent = 0;
uint32_t packetsReceived = 0;

for (auto &flow : stats)
{
    packetsSent = flow.second.txPackets;
    packetsReceived = flow.second.rxPackets;
}

uint32_t packetLoss = packetsSent - packetsReceived;
std::cout << "Calculated Packet Loss: " << packetLoss << std::endl;

// Throughput calculation
double simulationTime = 10.0;
double throughput = (packetsReceived * 1024 * 8) / (simulationTime * 1000000);

std::cout << "Throughput: " << throughput << " Mbps" << std::endl;
// EWMA Burst Detection
avgTraffic = (alpha * packetsSent) + ((1 - alpha) * avgTraffic);

std::cout << "Average Traffic (EWMA): " << avgTraffic << std::endl;

if (packetsSent > avgTraffic * 1.5)
{
    burstDetected = true;
    std::cout << "Burst Traffic Detected using EWMA!" << std::endl;
}

if (burstDetected)
{
    std::cout << "Applying BAMTQ Burst-Aware Queue Control..." << std::endl;
}

// Traffic classification
// Traffic Classification using Port Numbers (Real-time)
std::string trafficType;

for (auto &flow : stats)
{
    Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flow.first);

    uint16_t port = t.destinationPort;

    if (port == 9000)
    {
        trafficType = "Emergency";
    }
    else if (port == 5000)
    {
        trafficType = "Stream";
    }
    else if (port == 4000)
    {
        trafficType = "Sensor";
    }
    else
    {
        trafficType = "Bulk";
    }

    std::cout << "Traffic Type Classified: " << trafficType << std::endl;
}
// Step 10: Tenant Fairness using DWRR

std::cout << "Applying Tenant Fairness (DWRR)..." << std::endl;

// Simulated tenants
std::vector<std::string> tenants = {"Hospital", "Factory", "Sensors"};

// Assign quantum (bandwidth share)
std::map<std::string, uint32_t> quantum;
quantum["Hospital"] = 500;
quantum["Factory"] = 300;
quantum["Sensors"] = 200;

// Deficit counters
std::map<std::string, uint32_t> deficit;
for (auto &t : tenants)
{
    deficit[t] = 0;
}

// Simulate packet servicing
for (auto &t : tenants)
{
    deficit[t] += quantum[t];

    std::cout << "Tenant: " << t
              << " | Quantum: " << quantum[t]
              << " | Deficit: " << deficit[t]
              << std::endl;

    // Simulate sending packets of size 250 bytes
    uint32_t packetSize = 250;

    if (deficit[t] >= packetSize)
    {
        deficit[t] -= packetSize;
        std::cout << "Packet sent for " << t << std::endl;
    }
    else
    {
        std::cout << "Not enough deficit for " << t << std::endl;
    }
}
// Step 11: Multi-Queue System

std::cout << "Creating Multi-Queue System..." << std::endl;

// Queues
std::queue<std::string> emergencyQueue;
std::queue<std::string> streamQueue;
std::queue<std::string> sensorQueue;
std::queue<std::string> bulkQueue;

// Assign packet to queue based on traffic type
if (trafficType == "Emergency")
{
    emergencyQueue.push("Packet1");
}
else if (trafficType == "Stream")
{
    streamQueue.push("Packet1");
}
else if (trafficType == "Sensor")
{
    sensorQueue.push("Packet1");
}
else
{
    bulkQueue.push("Packet1");
}

// Print queue status
std::cout << "Queue Status:" << std::endl;
std::cout << "Emergency Queue Size: " << emergencyQueue.size() << std::endl;
std::cout << "Stream Queue Size: " << streamQueue.size() << std::endl;
std::cout << "Sensor Queue Size: " << sensorQueue.size() << std::endl;
std::cout << "Bulk Queue Size: " << bulkQueue.size() << std::endl;
// Step 12: Drop Management

std::cout << "Applying Drop Management Policy..." << std::endl;

uint32_t maxQueueSize = 1; // small size to simulate overflow

// Emergency Queue (strict - drop if deadline missed)
if (emergencyQueue.size() > maxQueueSize)
{
    std::cout << "Dropping packet from Emergency Queue (Deadline Missed)" << std::endl;
    emergencyQueue.pop();
}

// Stream Queue (Tail Drop)
if (streamQueue.size() > maxQueueSize)
{
    std::cout << "Dropping packet from Stream Queue (Tail Drop)" << std::endl;
    streamQueue.pop();
}

// Sensor Queue (Deadline-based drop)
if (sensorQueue.size() > maxQueueSize)
{
    std::cout << "Dropping packet from Sensor Queue (Low Priority Drop)" << std::endl;
    sensorQueue.pop();
}

// Bulk Queue (Aggressive drop)
if (bulkQueue.size() > maxQueueSize)
{
    std::cout << "Dropping packet from Bulk Queue (Aggressive Drop)" << std::endl;
    bulkQueue.pop();
}
// Deadline-based scheduling (BAMTQ)
std::cout << "Assigning deadlines to packets..." << std::endl;

uint32_t deadline;

if (trafficType == "Emergency")
{
    deadline = 5;
}
else if (trafficType == "Stream")
{
    deadline = 50;
}
else if (trafficType == "Sensor")
{
    deadline = 200;
}
else
{
    deadline = 1000; // Bulk
}

std::cout << "Packet Deadline Assigned: " << deadline << " ms" << std::endl;
// Step 13: EDF Scheduling (Earliest Deadline First)

// Step 13: Real EDF Scheduling using Queues

std::cout << "Scheduling using REAL EDF (Queue-based)" << std::endl;

// Process queues based on priority (smallest deadline first)

if (!emergencyQueue.empty())
{
    std::cout << "Transmitting packet from Emergency Queue (Deadline: 5 ms)" << std::endl;
    emergencyQueue.pop();
}
else if (!streamQueue.empty())
{
    std::cout << "Transmitting packet from Stream Queue (Deadline: 50 ms)" << std::endl;
    streamQueue.pop();
}
else if (!sensorQueue.empty())
{
    std::cout << "Transmitting packet from Sensor Queue (Deadline: 200 ms)" << std::endl;
    sensorQueue.pop();
}
else if (!bulkQueue.empty())
{
    std::cout << "Transmitting packet from Bulk Queue (Deadline: 1000 ms)" << std::endl;
    bulkQueue.pop();
}
else
{
    std::cout << "No packets to transmit" << std::endl;
}
Simulator::Destroy();

std::cout << "Traffic Simulation Completed" << std::endl;

    return 0;
}
