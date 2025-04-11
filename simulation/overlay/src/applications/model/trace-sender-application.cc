#include "trace-sender-application.h"

#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/random-variable-stream.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/boolean.h"

#include <cfloat>
#include <iostream>
#include <cinttypes>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TraceSender");

NS_OBJECT_ENSURE_REGISTERED (TraceSender);

TraceSender *
TraceFlowManager::GetTraceSender(Ptr<const Packet> packet) {
  TracePacketTag tag;
  if (packet->FindFirstMatchingByteTag(tag)) {
    return PacketToTraceSender(tag.GetId());
  } else {
    return 0;
  }
}

void
TraceFlowManager::DebugTracePacket(Ptr<const Packet> packet) {
  TracePacketTag tag;
  if (packet->FindFirstMatchingByteTag(tag)) { 
    if (packet_map.count(tag.GetId()) != 0) {
      std::cout << "Packet " << packet->GetUid() << ": Has tag and is mapped." << std::endl;
    } else {
      std::cout << "Packet " << packet->GetUid() << ": Has tag and is NOT mapped." << std::endl;
    }
  } else {
    std::cout << "Packet " << packet->GetUid() << ": Has NO tag!" << std::endl;
  }
}

TypeId
TraceSender::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TraceSender")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<TraceSender> ()
    .AddAttribute ("ProbeInterval", "Interval for sending trace probes (ms)",
                   UintegerValue (20),
                   MakeUintegerAccessor (&TraceSender::m_interval),
                   MakeUintegerChecker<uint32_t> (1))
    .AddAttribute ("SummaryInterval", "Interval for trace probe summary (x * ProbeInterval)",
                   UintegerValue (10),
                   MakeUintegerAccessor (&TraceSender::m_summary),
                   MakeUintegerChecker<uint32_t> (1))
    .AddAttribute ("PacketSize", "The size of packets sent in on state",
                   UintegerValue (1),
                   MakeUintegerAccessor (&TraceSender::m_pktSize),
                   MakeUintegerChecker<uint32_t> (1))
    .AddAttribute ("FromNode", "FromNode ID",
                   UintegerValue (0),
                   MakeUintegerAccessor (&TraceSender::m_fromNode),
                   MakeUintegerChecker<uint32_t> (0))
    .AddAttribute ("ToNode", "ToNode ID",
                   UintegerValue (0),
                   MakeUintegerAccessor (&TraceSender::m_toNode),
                   MakeUintegerChecker<uint32_t> (0))
    .AddAttribute ("Remote", "The address of the destination",
                   AddressValue (),
                   MakeAddressAccessor (&TraceSender::m_peer),
                   MakeAddressChecker ())
    .AddAttribute ("Local",
                   "The Address on which to bind the socket. If not set, it is generated automatically.",
                   AddressValue (),
                   MakeAddressAccessor (&TraceSender::m_local),
                   MakeAddressChecker ())
    .AddAttribute ("Protocol", "The type of protocol to use. This should be "
                   "a subclass of ns3::SocketFactory",
                   TypeIdValue (UdpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&TraceSender::m_tid),
                   MakeTypeIdChecker ())
    .AddAttribute ("EnableSeqTsSizeHeader",
                   "Enable use of SeqTsSizeHeader for sequence number and timestamp",
                   BooleanValue (false),
                   MakeBooleanAccessor (&TraceSender::m_enableSeqTsSizeHeader),
                   MakeBooleanChecker ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&TraceSender::m_txTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("TxWithAddresses", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&TraceSender::m_txTraceWithAddresses),
                     "ns3::Packet::TwoAddressTracedCallback")
    .AddTraceSource ("TxWithSeqTsSize", "A new packet is created with SeqTsSizeHeader",
                     MakeTraceSourceAccessor (&TraceSender::m_txTraceWithSeqTsSize),
                     "ns3::PacketSink::SeqTsSizeCallback")
  ;
  return tid;
}


TraceSender::TraceSender ()
  : m_socket (0),
    m_connected (false),
    m_totBytes (0)
{
  m_flowId = TraceFlowManager::GetInstance().RegisterTraceSender(this);
  NS_LOG_FUNCTION (this);
}

TraceElement::TraceElement ()
  : receive_time (0),
    received (false),
    hop_count (0)
{
  send_time = Simulator::Now().GetMicroSeconds();
  link_capacities = {};
  queue_capacities = {};
}

TraceSender::~TraceSender()
{
  NS_LOG_FUNCTION (this);
}

Ptr<Socket>
TraceSender::GetSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}

int64_t 
TraceSender::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  return 2;
}

void
TraceSender::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  CancelEvents ();
  m_socket = 0;
  Application::DoDispose ();
}

void TraceSender::StartApplication ()
{
  NS_LOG_FUNCTION (this);

  if (!m_socket)
    {
      m_socket = Socket::CreateSocket (GetNode (), m_tid);
      int ret = -1;

      if (! m_local.IsInvalid())
        {
          NS_ABORT_MSG_IF ((Inet6SocketAddress::IsMatchingType (m_peer) && InetSocketAddress::IsMatchingType (m_local)) ||
                           (InetSocketAddress::IsMatchingType (m_peer) && Inet6SocketAddress::IsMatchingType (m_local)),
                           "Incompatible peer and local address IP version");
          ret = m_socket->Bind (m_local);
        }
      else
        {
          if (Inet6SocketAddress::IsMatchingType (m_peer))
            {
              ret = m_socket->Bind6 ();
            }
          else if (InetSocketAddress::IsMatchingType (m_peer) ||
                   PacketSocketAddress::IsMatchingType (m_peer))
            {
              ret = m_socket->Bind ();
            }
        }

      if (ret == -1)
        {
          NS_FATAL_ERROR ("Failed to bind socket");
        }

      m_socket->Connect (m_peer);
      m_socket->SetAllowBroadcast (true);
      m_socket->ShutdownRecv ();

      m_socket->SetConnectCallback (
        MakeCallback (&TraceSender::ConnectionSucceeded, this),
        MakeCallback (&TraceSender::ConnectionFailed, this));
    }

  CancelEvents ();
  StartSending ();
}

void TraceSender::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  CancelEvents ();
  if(m_socket)
    {
      m_socket->Close ();
    }
  else
    {
      NS_LOG_WARN ("TraceSender found null socket to close in StopApplication");
    }
}

void TraceSender::CancelEvents ()
{
  NS_LOG_FUNCTION (this);

  Simulator::Cancel (m_sendEvent);
  Simulator::Cancel (m_startStopEvent);
}

void TraceSender::StartSending ()
{
  NS_LOG_FUNCTION (this);
  ScheduleNextTx ();
}

void TraceSender::ScheduleNextTx ()
{
  NS_LOG_FUNCTION (this);
  m_sendEvent = Simulator::Schedule (MilliSeconds(m_interval),
                                     &TraceSender::SendPacket, this);
}


void TraceSender::SendPacket ()
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT (m_sendEvent.IsExpired ());

  Ptr<Packet> probe_packet;
  if (m_enableSeqTsSizeHeader)
    {
      Address from, to;
      m_socket->GetSockName (from);
      m_socket->GetPeerName (to);
      SeqTsSizeHeader header;
      header.SetSeq (m_seq++);
      header.SetSize (m_pktSize);
      NS_ABORT_IF (m_pktSize < header.GetSerializedSize ());
      probe_packet = Create<Packet> (m_pktSize - header.GetSerializedSize ());
      m_txTraceWithSeqTsSize (probe_packet, from, to, header);
      probe_packet->AddHeader (header);
    }
  else
    {
      probe_packet = Create<Packet> (m_pktSize);
    }
  
  TracePacketTag tag(TraceFlowManager::GetInstance().RegisterTracePacket(m_flowId));
  probe_packet->AddByteTag(tag);
  Ptr<TraceElement> element = Create<TraceElement>();
  m_trace_map[tag.GetId()] = element;

  if (m_isFirstTransmission) {
    m_firstTransmission = element->send_time;
    m_isFirstTransmission = false;
  }

  int actual = m_socket->Send (probe_packet);
  if ((unsigned) actual == m_pktSize)
    {
      m_txTrace (probe_packet);
      m_totBytes += m_pktSize;
      Address localAddress;
      m_socket->GetSockName (localAddress);
      if (InetSocketAddress::IsMatchingType (m_peer))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                       << "s trace-sender sent "
                       << probe_packet->GetSize () << " bytes to "
                       << InetSocketAddress::ConvertFrom(m_peer).GetIpv4 ()
                       << " port " << InetSocketAddress::ConvertFrom (m_peer).GetPort ()
                       << " total Tx " << m_totBytes << " bytes");
          m_txTraceWithAddresses (probe_packet, localAddress, InetSocketAddress::ConvertFrom (m_peer));
        }
      else if (Inet6SocketAddress::IsMatchingType (m_peer))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                       << "s trace-sender sent "
                       << probe_packet->GetSize () << " bytes to "
                       << Inet6SocketAddress::ConvertFrom(m_peer).GetIpv6 ()
                       << " port " << Inet6SocketAddress::ConvertFrom (m_peer).GetPort ()
                       << " total Tx " << m_totBytes << " bytes");
          m_txTraceWithAddresses (probe_packet, localAddress, Inet6SocketAddress::ConvertFrom(m_peer));
        }
    }
  else
    {
      NS_LOG_INFO ("Unable Send trace packet!");
      NS_LOG_DEBUG ("Unable to send m_probe_packet; actual " << actual << " size " << m_pktSize << "; caching for later attempt");
    }
  ScheduleNextTx ();
}

void TraceSender::WriteResults(QueueAccumulationMethod mode) {
  std::vector<std::vector<std::pair<uint64_t, Ptr<TraceElement>>>> blocks;
  std::vector<std::pair<uint64_t, Ptr<TraceElement>>> currentBlock;

  for (const auto& entry : m_trace_map) {
    currentBlock.push_back(entry);
    if (currentBlock.size() == m_summary) {
      blocks.push_back(std::move(currentBlock));
      currentBlock.clear();
    }
  }

  if (currentBlock.size() != 0) {
    blocks.push_back(std::move(currentBlock));
  }

  std::ostringstream oss;
  oss << "trace_" << m_fromNode << "_to_" << m_toNode << ".csv";
  std::string filename =  oss.str();
  NS_LOG_UNCOND ("Wrting trace file to " << filename << " ... ");

  FILE* trace_csv = fopen(filename.c_str(), "w+");

  fprintf(trace_csv, "at,delay,stddev,min_link_cap,max_link_cap,queue_capacity,hops,dropratio\n");

  for (const auto& block : blocks) {
    if (block.size() == 0) continue;

    uint64_t delay = 0;
    uint64_t delay_entries = 0;
    double min_link_cap = 0;
    double max_link_cap  = 0;
    uint64_t queue_capacity = 0;
    uint32_t hops = block[0].second->hop_count;
    uint64_t drops = 0;

    for (const auto& entry : block) {
      auto& value = entry.second;

      double min_link_cap_search = DBL_MAX;
      double max_link_cap_search  = 0;
      if (value->receive_time != 0) {
        delay += value->receive_time - value->send_time;
        delay_entries++;
      } else {
        drops++;
      }

      long unsigned int bottleneck_at = 0;
      for (long unsigned int index = 0; index < value->link_capacities.size(); index++) {
        double link_cap = value->link_capacities[index];
        if (link_cap > max_link_cap_search) {
          max_link_cap_search = link_cap;
        }

        if (link_cap < min_link_cap_search) {
          min_link_cap_search = link_cap;
          bottleneck_at = index;
        }
      }

      min_link_cap = min_link_cap_search;
      max_link_cap = max_link_cap_search;

      if (value->queue_capacities.size() != value->link_capacities.size()) {
        NS_ABORT_MSG("Queue slot and link capacity slot mismatch!");
      }

      switch (mode) {
        case QueueAccumulationMethod::SUM:
          for (long unsigned int index = 0; index < value->queue_capacities.size(); index++) {
            queue_capacity += value->queue_capacities[index];
          }
          break;
        case QueueAccumulationMethod::BOTTLENECK:
          if (bottleneck_at >= value->queue_capacities.size()) {
            if (value->queue_capacities.size() == 0) {
              queue_capacity = 0;
            } else {
              queue_capacity = value->queue_capacities[value->queue_capacities.size() - 1];
            }
          } else {
            queue_capacity += value->queue_capacities[bottleneck_at];
          }
          break;
        case QueueAccumulationMethod::SUM_TO_BOTTLENECK:
          for (long unsigned int index = 0; index <= std::min(bottleneck_at, std::max(value->queue_capacities.size() - 1, 0UL)); index++) {
            queue_capacity += value->queue_capacities[index];
          }
          break;
      }
    }

    double dropratio = (double) drops / block.size();
    queue_capacity /= block.size();
    uint64_t avg_delay = 0;
    double std_dev = 0.0;
    if (delay_entries != 0) {
      avg_delay = delay / delay_entries;

      for (const auto& entry : block) {
        auto& value = entry.second;

        if (value->receive_time == 0) continue;

        uint64_t delay_b = value->receive_time - value->send_time;
        std_dev += pow((((double) delay_b - (double) avg_delay)), 2);
      }

      std_dev = sqrt(std_dev / delay_entries);
    } else {
      dropratio = 1.0;
    }

    fprintf(trace_csv, "%" PRIu64 ",%" PRIu64 ",%.2f,%.2f,%.2f,%" PRIu64 ",%" PRIu32 ",%.2f\n",
      block[0].second->send_time - m_firstTransmission,
      avg_delay,
      std_dev,
      min_link_cap,
      max_link_cap,
      queue_capacity,
      hops,
      dropratio
    );
  }

  fclose(trace_csv);
  NS_LOG_UNCOND ("Closed trace file " << filename);
}

void TraceSender::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  m_connected = true;
}

void TraceSender::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_FATAL_ERROR ("Can't connect");
}

void TraceSender::addHopDetails(Ptr<const Packet> packet, double link_load, uint64_t queue_cap, uint32_t at_node) {
  TracePacketTag tag;
  if (!packet->FindFirstMatchingByteTag(tag)) return;

  Ptr<TraceElement> element = m_trace_map.at(tag.GetId());

  if (at_node != m_node->GetId()) {
    element->queue_capacities.push_back(queue_cap);
  } else {
    element->queue_capacities.push_back(0);
  }
  element->link_capacities.push_back(link_load);
  element->transmit_times.push_back(Simulator::Now().GetMicroSeconds());
  element->hop_count++;
}

uint64_t TraceSender::getPacketId(Ptr<const Packet> packet) {
  TracePacketTag tag;
  if (!packet->FindFirstMatchingByteTag(tag)) {
    NS_LOG_ERROR ("Unable to get packet tag of packet " << packet->GetUid() << "!");
    return 0;
  }

  return tag.GetId();
}

void TraceSender::addReceptionDetails(Ptr<const Packet> packet, uint32_t at_node) {
  TracePacketTag tag;
  if (!packet->FindFirstMatchingByteTag(tag)) return;

  Ptr<TraceElement> element = m_trace_map.at(tag.GetId());
  element->receive_time = Simulator::Now().GetMicroSeconds();
  element->received = true;
}

void TraceSender::debugTraceElement(Ptr<const Packet> packet) {
  TracePacketTag tag;
  if (!packet->FindFirstMatchingByteTag(tag)) return;

  Ptr<TraceElement> element = m_trace_map.at(tag.GetId());
  NS_LOG_INFO ("--- TracePacket id=" << tag.GetId() << ", flow=" << m_flowId);
  NS_LOG_INFO ("Sended: " << element->send_time << ", Received: " << element->receive_time << ", Took: " << element->receive_time - element->send_time);
  NS_LOG_INFO ("-----------------------");
}

} // Namespace ns3
