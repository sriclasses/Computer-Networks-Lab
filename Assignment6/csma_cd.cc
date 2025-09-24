#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include <fstream>
#include <cmath>

using namespace ns3;

// Define a logging component to help observe messages in simulation output
NS_LOG_COMPONENT_DEFINE("CsmaCdSimulation");
// NS_LOG_COMPONENT_DEFINE ("CSMACDExample");

// Declare a FlowMonitor pointer for capturing and monitoring flow statistics
Ptr<FlowMonitor> monitor;
FlowMonitorHelper flowmon;

// Function to aggregate network statistics at each time interval and write them to a CSV file
void WriteFlowStatsToCSV(double currentTime) {
    // Open CSV file in append mode to write statistics
    std::ofstream csvFile;
    csvFile.open("network_metrics.csv", std::ios::app);  // Append mode
    if (!csvFile.is_open()) {
        NS_LOG_ERROR("Failed to open CSV file"); // Log error if file couldn't be opened
        return;
    }

    // Gather and check flow statistics at the current simulation time
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    // Initialize aggregate values to store total statistics across all flows
    double totalThroughput = 0.0;
    double totalPacketLoss = 0.0;
    double totalAvgDelay = 0.0;
    uint32_t totalTxPackets = 0;
    uint32_t totalRxPackets = 0;

    // Loop through each flow's statistics to aggregate values
    for (auto iter = stats.begin(); iter != stats.end(); ++iter) {
        totalTxPackets += iter->second.txPackets;  // Count total transmitted packets
        totalRxPackets += iter->second.rxPackets;  // Count total received packets
        totalThroughput += iter->second.rxBytes * 8.0 / currentTime / 1e6; // Compute throughput in Mbps
        totalPacketLoss += (1 - ((double)iter->second.rxPackets / iter->second.txPackets)) * 100;  // Compute packet loss percentage
        totalAvgDelay += iter->second.rxPackets > 0 ? iter->second.delaySum.GetSeconds() / iter->second.rxPackets : 0;  // Average delay calculation
    }

    // Calculate the average values for throughput, packet loss, and delay across all flows
    double avgThroughput = totalThroughput / stats.size();
    double avgPacketLossRatio = totalPacketLoss / stats.size();
    double avgDelay = totalRxPackets > 0 ? totalAvgDelay / stats.size() : 0;

    // Write the aggregated data to the CSV file at the current simulation time
    csvFile << currentTime << ","
            << totalTxPackets << "," << totalRxPackets << ","
            << avgThroughput << "," << avgPacketLossRatio << "," << avgDelay << "\n";

    // Close the CSV file
    csvFile.close();
}

int main(int argc, char *argv[]) {
        // Step 1: Enable logging for relevant components
    LogComponentEnable("CsmaHelper", LOG_LEVEL_INFO);  // Enable CSMA Helper log
    LogComponentEnable("NetDevice", LOG_LEVEL_INFO);   // Enable NetDevice log
    LogComponentEnable("FlowMonitor", LOG_LEVEL_INFO); // Enable FlowMonitor log
    LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
    LogComponentEnable("CsmaHelper", LOG_LEVEL_INFO);
    LogComponentEnable("CsmaNetDevice", LOG_LEVEL_ALL);
    LogComponentEnable("CsmaChannel", LOG_LEVEL_ALL);
    // LogComponentEnableAll(LOG_LEVEL_INFO);

    // Simulation parameters
    uint32_t nNodes = 5;            // Number of nodes in the network
    double simulationTime = 3.0;    // Total duration of simulation in seconds
    uint32_t packetSize = 1024;      // Size of each packet in bytes
    std::string dataRate = "100Mbps"; // Data rate of the CSMA channel
    std::string delay = "0.1ms";     // Propagation delay of the CSMA channel

    // Parse command-line arguments if provided
    CommandLine cmd;
    cmd.Parse(argc, argv);

    // Create a container to hold all the nodes in the network
    NodeContainer nodes;
    nodes.Create(nNodes);

    // Configure the CSMA channel with specified data rate and delay
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue(dataRate));
    csma.SetChannelAttribute("Delay", StringValue(delay));

    // Install CSMA network devices onto the nodes
    NetDeviceContainer devices = csma.Install(nodes);

    // Install the internet stack on the nodes to enable IP-based communication
    InternetStackHelper internet;
    internet.Install(nodes);

    // Assign IP addresses to each device connected to the CSMA channel
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

    // Set up OnOff applications to simulate traffic between nodes
    OnOffHelper onOff("ns3::UdpSocketFactory", Address());
    onOff.SetConstantRate(DataRate("50Mbps"), packetSize); // Configure traffic rate and packet size

    // Install applications on each node, setting their remote targets cyclically
    for (uint32_t i = 0; i < nNodes; i++) {
        // Define remote address for each node's traffic destination
        AddressValue remoteAddress(InetSocketAddress(interfaces.GetAddress((i + 1) % nNodes), 8080));
        onOff.SetAttribute("Remote", remoteAddress);
        ApplicationContainer app = onOff.Install(nodes.Get(i));  // Install app on node
        app.Start(Seconds(1.0));                                 // Start app at 1 second
        app.Stop(Seconds(simulationTime));                       // Stop app at simulation end
    }

    // Enable logging for this simulation to observe CSMA/CD operations
    LogComponentEnable("CsmaCdSimulation", LOG_LEVEL_INFO);

    // Set up FlowMonitor to capture network statistics throughout the simulation
    monitor = flowmon.InstallAll();

    // Create and open the CSV file to write headers for network metrics
    std::ofstream csvFile("network_metrics.csv");
    csvFile << "Time (s),Tx Packets,Rx Packets,Throughput (Mbps),Packet Loss Ratio (%),Average Delay (s)\n";
    csvFile.close();

    // Schedule the WriteFlowStatsToCSV function to run at regular intervals throughout the simulation
    for (double time = 1.0; time <= simulationTime; time += 0.25) {
        Simulator::Schedule(Seconds(time), &WriteFlowStatsToCSV, time);
    }

    // Run the simulation
    Simulator::Stop(Seconds(simulationTime));  // Stop the simulation at the designated time
    Simulator::Run();                          // Execute the scheduled events

    // Cleanup and destroy the simulator after completion
    Simulator::Destroy();
    return 0;
}
