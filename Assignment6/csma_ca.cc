#include "ns3/command-line.h"  // Command-line argument parsing for the simulation
#include "ns3/config.h"         // Configuring parameters for various NS-3 components
#include "ns3/double.h"         // Handling double type values
#include "ns3/flow-monitor-helper.h" // Flow monitoring to track packet flow details
#include "ns3/internet-stack-helper.h" // Internet stack installation for routing
#include "ns3/ipv4-address-helper.h"  // IPv4 addressing helper to assign addresses
#include "ns3/ipv4-flow-classifier.h" // Classifies flows based on 5-tuple
#include "ns3/ipv4-list-routing-helper.h" // Allows adding multiple routing protocols
#include "ns3/ipv4-static-routing-helper.h" // For static routing configuration
#include "ns3/log.h"            // Log messages for debugging
#include "ns3/mobility-helper.h" // For node mobility configuration
#include "ns3/mobility-model.h" // Mobility model for nodes
#include "ns3/olsr-helper.h"    // OLSR routing protocol helper
#include "ns3/string.h"         // For string handling
#include "ns3/uinteger.h"       // For integer handling
#include "ns3/yans-wifi-channel.h" // For wireless channel configuration
#include "ns3/yans-wifi-helper.h"   // Helper for configuring Wifi device

using namespace ns3;  // Use ns3 namespace to avoid fully qualified names

// Define a log component for easier logging
NS_LOG_COMPONENT_DEFINE("WifiSimpleAdhocGrid");

// Function to handle receiving packets
void ReceivePacket(Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    // Continuously receive packets while they are available
    while ((packet = socket->Recv()))
    {
        // Get the node associated with the socket (which is receiving the packet)
        Ptr<Node> node = socket->GetNode();
        
        // Log the node ID that is receiving the packet
        NS_LOG_UNCOND("Node " << node->GetId() << " received a packet!");
    }
}

// Function to generate traffic (packets) on the network
static void GenerateTraffic(Ptr<Socket> socket, uint32_t pktSize, uint32_t pktCount, Time pktInterval)
{
    if (pktCount > 0)
    {
        // Create a new packet and send it using the socket
        socket->Send(Create<Packet>(pktSize));
        // Schedule the next packet generation
        Simulator::Schedule(pktInterval, &GenerateTraffic, socket, pktSize, pktCount - 1, pktInterval);
    }
    else
    {
        // Close the socket when no more packets need to be sent
        socket->Close();
    }
}

int main(int argc, char* argv[])
{
    // Default simulation parameters
    std::string phyMode{"DsssRate11Mbps"};  // Wifi physical layer mode
    double distance{40};  // Distance between nodes in meters
    uint32_t packetSize{100}; // Size of each packet in bytes
    uint32_t numPackets{1000}; // Total number of packets to be sent
    uint32_t numNodes{25}; // Number of nodes (arranged in a 5x5 grid)
    bool verbose{false}; // Flag to enable verbose logging
    bool tracing{true};  // Flag to enable ASCII and pcap tracing
    Time interPacketInterval{"0.01s"}; // Interval between consecutive packets

    // Command line argument parsing setup
    CommandLine cmd(__FILE__);
    cmd.AddValue("phyMode", "Wifi Phy mode", phyMode);  // Argument for Wifi Phy mode
    cmd.AddValue("distance", "distance (m)", distance);  // Argument for distance between nodes
    cmd.AddValue("packetSize", "size of application packet sent", packetSize);  // Argument for packet size
    cmd.AddValue("numPackets", "number of packets generated", numPackets);  // Argument for number of packets
    cmd.AddValue("interval", "interval (seconds) between packets", interPacketInterval);  // Interval between packets
    cmd.AddValue("verbose", "turn on all WifiNetDevice log components", verbose);  // Enable verbose logging
    cmd.AddValue("tracing", "turn on ascii and pcap tracing", tracing);  // Enable tracing
    cmd.AddValue("numNodes", "number of nodes", numNodes);  // Argument for number of nodes
    cmd.Parse(argc, argv);  // Parse command-line arguments

    // Set default Wifi remote station manager configuration
    Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue(phyMode));
    // Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue(20)); // Enable RTS/CTS with a threshold

    // Create a container for nodes
    NodeContainer c;
    c.Create(numNodes);

    // Setup Wifi helper and enable logging if verbose flag is set
    WifiHelper wifi;
    if (verbose)
    {
        WifiHelper::EnableLogComponents();
    }

    // Setup the physical layer for Wifi devices
    YansWifiPhyHelper wifiPhy;
    wifiPhy.Set("RxGain", DoubleValue(-10)); // Set the receive gain (signal strength)
    wifiPhy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO); // Set the pcap data link type

    // Setup the Wifi channel (propagation and loss model)
    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel"); // Constant propagation delay model
    wifiChannel.AddPropagationLoss("ns3::FriisPropagationLossModel"); // Friis loss model for free-space propagation
    wifiPhy.SetChannel(wifiChannel.Create());  // Apply the channel configuration

    // Setup Wifi MAC (Medium Access Control) layer for ad hoc mode
    WifiMacHelper wifiMac;
    wifi.SetStandard(WIFI_STANDARD_80211b);  // Set Wifi standard to 802.11b
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode", StringValue(phyMode), "ControlMode", StringValue(phyMode)); // Set data rate
    wifiMac.SetType("ns3::AdhocWifiMac");  // Set MAC type to AdHoc (no infrastructure mode)

    // Install Wifi devices on the nodes
    NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, c);

    // Setup node mobility (positions in a grid)
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator", "MinX", DoubleValue(0.0), "MinY", DoubleValue(0.0), "DeltaX", DoubleValue(distance), "DeltaY", DoubleValue(distance), "GridWidth", UintegerValue(5), "LayoutType", StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");  // Nodes are stationary
    mobility.Install(c);

    // Setup routing protocols (OLSR and Static routing)
    OlsrHelper olsr;  // OLSR (Optimized Link State Routing) helper
    Ipv4StaticRoutingHelper staticRouting;  // Static routing helper
    Ipv4ListRoutingHelper list;
    list.Add(staticRouting, 0);  // Add static routing at priority 0
    list.Add(olsr, 10);  // Add OLSR at priority 10

    // Install the Internet stack (IP routing, TCP, UDP, etc.) on nodes
    InternetStackHelper internet;
    internet.SetRoutingHelper(list);
    internet.Install(c);

    // Set up IPv4 addressing and assign IP addresses to devices
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");  // Define network address and subnet mask
    Ipv4InterfaceContainer i = ipv4.Assign(devices);  // Assign IPs to devices

    // Setup the socket type for UDP communication
    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");

    // Create receiver sockets for each flow
    Ptr<Socket> recvSink1 = Socket::CreateSocket(c.Get(24), tid); // Receiver socket on node 24 for Flow 1
    InetSocketAddress local1 = InetSocketAddress(Ipv4Address::GetAny(), 9); // Bind to any address on port 9
    recvSink1->Bind(local1);  // Bind the socket
    recvSink1->SetRecvCallback(MakeCallback(&ReceivePacket)); // Set callback for packet reception

    Ptr<Socket> recvSink2 = Socket::CreateSocket(c.Get(20), tid); // Receiver socket on node 20 for Flow 2
    InetSocketAddress local2 = InetSocketAddress(Ipv4Address::GetAny(), 9); // Bind to any address on port 9
    recvSink2->Bind(local2);  // Bind the socket
    recvSink2->SetRecvCallback(MakeCallback(&ReceivePacket)); // Set callback for packet reception

    Ptr<Socket> recvSink3 = Socket::CreateSocket(c.Get(14), tid); // Receiver socket on node 14 for Flow 3
    InetSocketAddress local3 = InetSocketAddress(Ipv4Address::GetAny(), 9); // Bind to any address on port 9
    recvSink3->Bind(local3);  // Bind the socket
    recvSink3->SetRecvCallback(MakeCallback(&ReceivePacket)); // Set callback for packet reception

    // Create source sockets for each flow (senders)
    Ptr<Socket> source1 = Socket::CreateSocket(c.Get(0), tid);  // Source socket on node 0 for Flow 1
    InetSocketAddress remote1 = InetSocketAddress(i.GetAddress(24, 0), 9);  // Destination is node 24, port 9
    source1->Connect(remote1);  // Connect to the destination

    Ptr<Socket> source2 = Socket::CreateSocket(c.Get(4), tid);  // Source socket on node 4 for Flow 2
    InetSocketAddress remote2 = InetSocketAddress(i.GetAddress(20, 0), 9);  // Destination is node 20, port 9
    source2->Connect(remote2);  // Connect to the destination

    Ptr<Socket> source3 = Socket::CreateSocket(c.Get(10), tid);  // Source socket on node 10 for Flow 3
    InetSocketAddress remote3 = InetSocketAddress(i.GetAddress(14, 0), 9);  // Destination is node 14, port 9
    source3->Connect(remote3);  // Connect to the destination

    // Schedule traffic generation for each source at different times
    Simulator::Schedule(Seconds(1.0), &GenerateTraffic, source1, packetSize, numPackets, interPacketInterval);
    Simulator::Schedule(Seconds(1.5), &GenerateTraffic, source2, packetSize, numPackets, interPacketInterval);
    Simulator::Schedule(Seconds(2.0), &GenerateTraffic, source3, packetSize, numPackets, interPacketInterval);

    // Enable tracing (Ascii and pcap files)
    if (tracing)
    {
        AsciiTraceHelper ascii;
        wifiPhy.EnableAsciiAll(ascii.CreateFileStream("wifi-simple-adhoc-grid.tr"));  // ASCII trace
    }

    // Flow monitor setup
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();  // Install flow monitor on all nodes

    // Run the simulation for 100 seconds
    Simulator::Stop(Seconds(100.0));
    Simulator::Run();

    // Check for lost packets and display flow statistics
    monitor->CheckForLostPackets();
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats(); // Get statistics for all flows
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());  // Flow classifier to identify flow pairs
    if (classifier)
    {
        for (auto& statsEntry : stats)
        {
            // Access the flow's 5-tuple and stats
            Ipv4FlowClassifier::FiveTuple fiveTuple = classifier->FindFlow(statsEntry.first);
            FlowMonitor::FlowStats flowStats = statsEntry.second;

            // Print out flow statistics (packets sent/received, delay, throughput)
            std::cout << "Flow from " << fiveTuple.sourceAddress << " to " << fiveTuple.destinationAddress << "\n";
            std::cout << "  Packets sent: " << flowStats.txPackets << "\n";
            std::cout << "  Packets received: " << flowStats.rxPackets << "\n";
            std::cout << "  Lost packets: " << flowStats.lostPackets << "\n";

            // Delay calculation (avoid division by zero)
            if (flowStats.rxPackets > 0)
            {
                std::cout << "  Delay: " << flowStats.delaySum.GetSeconds() / flowStats.rxPackets << " seconds\n";
            }
            else
            {
                std::cout << "  Delay: N/A (no packets received)\n";
            }

            // Throughput calculation (avoid division by zero)
            if (flowStats.rxPackets > 0)
            {
                double throughput = flowStats.rxBytes * 8.0 / (flowStats.timeLastRxPacket.GetSeconds() - flowStats.timeFirstTxPacket.GetSeconds()) / 1000;
                std::cout << "  Throughput: " << throughput << " kbps\n";
            }
            else
            {
                std::cout << "  Throughput: N/A (no packets received)\n";
            }
        }
    }

    // Cleanup and end the simulation
    Simulator::Destroy();
    return 0;
}
