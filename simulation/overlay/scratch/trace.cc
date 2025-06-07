#include "ns3/core-module.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/trace-sender-helper.h"
#include "ns3/trace-receiver-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TraceGenerator");

int main(int argc, char* argv[]) {
    std::string bottleneckRate = "30Mbps";
    uint64_t bottleneckDelayMilliSeconds = 10;
    uint16_t fromNode = 0;
    uint16_t toNode = 2;
    uint64_t runtimeSeconds = 120;
    bool useTcp = false;
    uint32_t seed = 123456789;

    CommandLine cmd(__FILE__);
    cmd.AddValue("bottleneckRate", "Data rate of the bottleneck link", bottleneckRate);
    cmd.AddValue("bottleneckDelay", "Nanosecond delay of the bottleneck link", bottleneckDelayMilliSeconds);
    cmd.AddValue("from", "Trace file from node ID", fromNode);
    cmd.AddValue("to", "Trace file to node ID", toNode);
    cmd.AddValue("runtime", "Trace generation runtime seconds", runtimeSeconds);
    cmd.AddValue("tcp", "Use TCP instead of UDP for cross traffic", useTcp);
    cmd.AddValue("seed", "RNG seed", seed);
    cmd.Parse(argc, argv);

    std::string socketFactory = "ns3::UdpSocketFactory";
    if (useTcp) {
        NS_LOG_UNCOND("WARNING: Using TCP for crosstraffic!");
        socketFactory = "ns3::TcpSocketFactory";
    }

    SeedManager::SetSeed(seed);

    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpCubic"));
    Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue(6291456));
    Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue(6291456));

    NodeContainer nodes;
    nodes.Create(4);

    NodeContainer bridges;
    bridges.Create(2);

    PointToPointHelper defaultHelper;
    defaultHelper.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    defaultHelper.SetChannelAttribute("Delay", TimeValue(NanoSeconds(100)));
    defaultHelper.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", QueueSizeValue(QueueSize("100p")));

    PointToPointHelper bottleneckHelper;
    bottleneckHelper.SetDeviceAttribute("DataRate", StringValue(bottleneckRate));
    bottleneckHelper.SetChannelAttribute("Delay", TimeValue(MilliSeconds(bottleneckDelayMilliSeconds)));
    bottleneckHelper.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", QueueSizeValue(QueueSize("100p")));

    NetDeviceContainer n0toBridge0 = defaultHelper.Install(NodeContainer(nodes.Get(0), bridges.Get(0)));
    NetDeviceContainer n1toBridge0 = defaultHelper.Install(NodeContainer(nodes.Get(1), bridges.Get(0)));
    NetDeviceContainer n2toBridge1 = defaultHelper.Install(NodeContainer(nodes.Get(2), bridges.Get(1)));
    NetDeviceContainer n3toBridge1 = defaultHelper.Install(NodeContainer(nodes.Get(3), bridges.Get(1)));
    NetDeviceContainer bridgeLink = bottleneckHelper.Install(bridges);

    InternetStackHelper stackHelper;
    stackHelper.Install(nodes);
    stackHelper.Install(bridges);

    /*
     * Node 0            Bridge 0          Bridge 1          Node 2  
     * 10.0.1.1 <------> 10.0.1.2          10.0.3.2 <------> 10.0.3.1
     *                                                               
     *                  10.0.10.1 <------> 10.0.10.2                 
     * Node 1                                                Node 3  
     * 10.0.2.1 <------> 10.0.2.2          10.0.4.2 <------> 10.0.4.1
     */

    Ipv4AddressHelper ipv4Helper;
    ipv4Helper.SetBase("10.0.1.0", "255.255.255.0");
    ipv4Helper.Assign(n0toBridge0);
    ipv4Helper.SetBase("10.0.2.0", "255.255.255.0");
    ipv4Helper.Assign(n1toBridge0);
    ipv4Helper.SetBase("10.0.3.0", "255.255.255.0");
    ipv4Helper.Assign(n2toBridge1);
    ipv4Helper.SetBase("10.0.4.0", "255.255.255.0");
    ipv4Helper.Assign(n3toBridge1);
    ipv4Helper.SetBase("10.0.10.0", "255.255.255.0");
    ipv4Helper.Assign(bridgeLink);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // n0 -> n2
    OnOffHelper onoff(socketFactory, Address(InetSocketAddress(Ipv4Address("10.0.3.1"), 1000)));
    onoff.SetConstantRate(DataRate("10Mbps"));
    ApplicationContainer app = onoff.Install(nodes.Get(0));
    app.Start(Seconds(20));
    app.Stop(Seconds(100));

    PacketSinkHelper sink(socketFactory, Address(InetSocketAddress(Ipv4Address::GetAny(), 1000)));
    app = sink.Install(nodes.Get(2));
    app.Start(Seconds(0));

    // n1 -> n3
    onoff.SetAttribute("Remote", AddressValue(InetSocketAddress(Ipv4Address("10.0.4.1"), 1000)));
    app = onoff.Install(nodes.Get(1));
    app.Start(Seconds(40));
    app.Stop(Seconds(80));

    app = sink.Install(nodes.Get(3));
    app.Start(Seconds(0));

    // n2 -> n0
    onoff.SetAttribute("Remote", AddressValue(InetSocketAddress(Ipv4Address("10.0.2.1"), 1001)));
    app = onoff.Install(nodes.Get(2));
    app.Start(Seconds(20));
    app.Stop(Seconds(100));

    sink.SetAttribute("Local", AddressValue(InetSocketAddress(Ipv4Address::GetAny(), 1001)));
    app = sink.Install(nodes.Get(0));
    app.Start(Seconds(0));

    // n3 -> n1
    onoff.SetAttribute("Remote", AddressValue(InetSocketAddress(Ipv4Address("10.0.1.1"), 1001)));
    app = onoff.Install(nodes.Get(3));
    app.Start(Seconds(40));
    app.Stop(Seconds(80));

    app = sink.Install(nodes.Get(1));
    app.Start(Seconds(0));

    // Install Tracers
    Ptr<Node> fromNodePtr = nodes.Get(fromNode);
    Ptr<Node> toNodePtr = nodes.Get(toNode);
    int64_t trace_interval = 40; // each 40ms
    int64_t trace_summary = 5; // average 5 measurements = 200ms trace resolution
    
    ApplicationContainer traceSenderApps;
    ApplicationContainer traceReceiverApps;

    Address toNodeAddress(InetSocketAddress(toNodePtr->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(), 1020));
    TraceSenderHelper fromTraceSender(toNodeAddress, fromNode, toNode);
    fromTraceSender.SetAttribute("Protocol", StringValue("ns3::UdpSocketFactory"));
    fromTraceSender.SetAttribute("ProbeInterval", UintegerValue(trace_interval));
    fromTraceSender.SetAttribute("SummaryInterval", UintegerValue(trace_summary));
    traceSenderApps.Add(fromTraceSender.Install(fromNodePtr));
    TraceReceiverHelper toTraceReceiver(toNodeAddress);
    toTraceReceiver.SetAttribute("Protocol", StringValue("ns3::UdpSocketFactory"));
    traceReceiverApps.Add(toTraceReceiver.Install(toNodePtr));

    Address fromNodeAddress(InetSocketAddress(fromNodePtr->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(), 1021));
    TraceSenderHelper toTraceSender(fromNodeAddress, toNode, fromNode);
    toTraceSender.SetAttribute("Protocol", StringValue("ns3::UdpSocketFactory"));
    toTraceSender.SetAttribute("ProbeInterval", UintegerValue(trace_interval));
    toTraceSender.SetAttribute("SummaryInterval", UintegerValue(trace_summary));
    traceSenderApps.Add(toTraceSender.Install(toNodePtr));
    TraceReceiverHelper fromTraceReceiver(fromNodeAddress);
    fromTraceReceiver.SetAttribute("Protocol", StringValue("ns3::UdpSocketFactory"));
    traceReceiverApps.Add(fromTraceReceiver.Install(fromNodePtr));
    
    traceReceiverApps.Start(MilliSeconds(0));

    Ptr<UniformRandomVariable> startJitter = CreateObject<UniformRandomVariable>();
    startJitter->SetAttribute("Min", DoubleValue(0.0));
    startJitter->SetAttribute("Max", DoubleValue(trace_interval));
    for (uint32_t i = 0; i < traceSenderApps.GetN(); i++) {
        ApplicationContainer app;
        app.Add(traceSenderApps.Get(i));
        app.Start(MilliSeconds(startJitter->GetInteger()));
    }

    Simulator::Stop(Seconds(runtimeSeconds));
    Simulator::Run();

    for (uint32_t i = 0; i < traceSenderApps.GetN(); i++) {
        Ptr<Application> app = traceSenderApps.Get(i);
        Ptr<TraceSender> traceSender = DynamicCast<TraceSender>(app);

        if (traceSender) {
            traceSender->WriteResults(QueueAccumulationMethod::BOTTLENECK);
        } else {
            NS_LOG_ERROR ("Application is not of type TraceSender!");
        }
    }

    Simulator::Destroy();

    return 0;
}
