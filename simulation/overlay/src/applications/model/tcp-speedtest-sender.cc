#include "tcp-speedtest-sender.h"

#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/socket.h"
#include "ns3/string.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/tcp-tx-buffer.h"

#include <fstream>
#include <cinttypes>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TCPSpeedtestSender");

NS_OBJECT_ENSURE_REGISTERED (TCPSpeedtestSender);

TCPSpeedtestSender *
SpeedtestManager::GetSender(Ptr<const Packet> packet) {
  SpeedtestTag tag;
  if (packet->FindFirstMatchingByteTag(tag)) {
    return PacketToSender(tag.GetId());
  } else {
    return 0;
  }
}

void
SpeedtestManager::DebugPacket(Ptr<const Packet> packet) {
  SpeedtestTag tag;
  if (packet->FindFirstMatchingByteTag(tag)) { 
    if (packet_map.count(tag.GetId()) != 0) {
      NS_LOG_INFO ("Packet " << packet->GetUid() << ": Has tag " << tag.GetId() << " and is mapped.");
    } else {
      NS_LOG_INFO ("Packet " << packet->GetUid() << ": Has tag " << tag.GetId() << " and is NOT mapped, size=" << packet->GetSize());
    }
  } else {
    NS_LOG_INFO ("Packet " << packet->GetUid() << ": Has NO tag!");
  }
}

TypeId
TCPSpeedtestSender::GetTypeId(void) {
    static TypeId tid = TypeId("ns3::TCPSpeedtestSender")
            .SetParent<Application>()
            .SetGroupName("Applications")
            .AddConstructor<TCPSpeedtestSender>()
            .AddAttribute("NoLimit", "Do not limit the data rate, let TCP handle stuff.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&TCPSpeedtestSender::m_noLimit),
                          MakeBooleanChecker())
            .AddAttribute("TrackAtDevice", "Track E2E delay at NetDevice level (instead of socket).",
                          BooleanValue(false),
                          MakeBooleanAccessor(&TCPSpeedtestSender::m_trackAtDev),
                          MakeBooleanChecker())
            .AddAttribute("DataRate", "The data rate to send packets with.",
                          DataRateValue(DataRate("20Mb/s")),
                          MakeDataRateAccessor(&TCPSpeedtestSender::m_dataRate),
                          MakeDataRateChecker())
            .AddAttribute("PacketSize", "The size of individual packets.",
                          UintegerValue(1024),
                          MakeUintegerAccessor(&TCPSpeedtestSender::m_packetSize),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("Remote", "The address of the destination",
                          AddressValue(),
                          MakeAddressAccessor(&TCPSpeedtestSender::m_peer),
                          MakeAddressChecker())
            .AddAttribute("Protocol", "The type of protocol to use.",
                          TypeIdValue(TcpSocketFactory::GetTypeId()),
                          MakeTypeIdAccessor(&TCPSpeedtestSender::m_tid),
                          MakeTypeIdChecker())
            .AddTraceSource("Tx", "A new packet is created and is sent",
                            MakeTraceSourceAccessor(&TCPSpeedtestSender::m_txTrace),
                            "ns3::Packet::TracedCallback")
            .AddAttribute("ReceiverID", "ID of the expected receiver node.",
                            UintegerValue(0),
                            MakeUintegerAccessor(&TCPSpeedtestSender::m_receiverId),
                            MakeUintegerChecker<uint32_t>());
    return tid;
}

SpeedtestEntry::SpeedtestEntry ()
  : sendTime (0),
    receiveTime (0),
    socketRTT (0),
    progress (0),
    cwnd (0) {}

TCPSpeedtestSender::TCPSpeedtestSender()
        : m_socket(0),
          m_connected(false),
          m_totBytes(0),
          m_completionTimeNs(-1),
          m_connFailed(false),
          m_closedNormally(false),
          m_closedByError(false),
          m_ackedBytes(0),
          m_isCompleted(false),
          m_current_cwnd_byte(0),
          m_current_rtt_ns(0),
          m_cancel(false),
          m_reusePacketId(0) {
    NS_LOG_FUNCTION(this);

    m_flowId = SpeedtestManager::GetInstance().RegisterSender(this);
}

TCPSpeedtestSender::~TCPSpeedtestSender() {
    NS_LOG_FUNCTION(this);
}

void
TCPSpeedtestSender::DoDispose(void) {
    NS_LOG_FUNCTION(this);

    m_socket = 0;
    Application::DoDispose();
}

void TCPSpeedtestSender::StartApplication(void) {
    NS_LOG_FUNCTION(this);

    if (!m_socket) {
        m_socket = Socket::CreateSocket(GetNode(), m_tid);

        if (m_socket->GetSocketType() != Socket::NS3_SOCK_STREAM &&
            m_socket->GetSocketType() != Socket::NS3_SOCK_SEQPACKET) {
            NS_FATAL_ERROR("Using TcpFlowSendApplication with an incompatible socket type. "
                           "TcpFlowSendApplication requires SOCK_STREAM or SOCK_SEQPACKET. "
                           "In other words, use TCP instead of UDP.");
        }

        if (Inet6SocketAddress::IsMatchingType(m_peer)) {
            if (m_socket->Bind6() == -1) {
                NS_FATAL_ERROR("Failed to bind socket");
            }
        } else if (InetSocketAddress::IsMatchingType(m_peer)) {
            if (m_socket->Bind() == -1) {
                NS_FATAL_ERROR("Failed to bind socket");
            }
        }

        m_socket->Connect(m_peer);
        m_socket->ShutdownRecv();

        m_socket->SetConnectCallback(
                MakeCallback(&TCPSpeedtestSender::ConnectionSucceeded, this),
                MakeCallback(&TCPSpeedtestSender::ConnectionFailed, this)
        );
        m_socket->SetSendCallback(MakeCallback(&TCPSpeedtestSender::DataSend, this));
        m_socket->SetCloseCallbacks(
                MakeCallback(&TCPSpeedtestSender::SocketClosedNormal, this),
                MakeCallback(&TCPSpeedtestSender::SocketClosedError, this)
        );

        m_socket->TraceConnectWithoutContext("CongestionWindow", MakeCallback(&TCPSpeedtestSender::CwndChange, this));
        m_socket->TraceConnectWithoutContext("RTT", MakeCallback(&TCPSpeedtestSender::RttChange, this));
    }
   
    if (m_connected) {
        if (m_noLimit) {
            Simulator::Schedule(MilliSeconds(1), &TCPSpeedtestSender::SendDataForcedNoLimit, this);
        } else {
            ScheduleNextTx();
        }
    }
}

void TCPSpeedtestSender::StopApplication(void) {
    NS_LOG_FUNCTION(this);
    m_cancel = true;
    if (m_socket != nullptr) {
        m_socket->Close();
        m_connected = false;
    } else {
        NS_LOG_WARN("TCPSpeedtestSender found null socket to close in StopApplication");
    }
}

void TCPSpeedtestSender::ScheduleNextTx() {
    if (m_cancel) return;
    uint32_t bits = m_packetSize * 8;
    Time nextTime(Seconds(bits / static_cast<double>(m_dataRate.GetBitRate())));
    Simulator::Schedule(nextTime, &TCPSpeedtestSender::SendData, this);
}

void TCPSpeedtestSender::SendDataForcedNoLimit() {
    NS_LOG_FUNCTION(this);

    if (m_cancel) return;
    while (true) {
        Ptr<Packet> packet;
        
        packet = Create<Packet>(m_packetSize);
        uint64_t packetId = 0;
        Ptr<SpeedtestEntry> entry;
        if (m_reusePacketId == 0) {
            packetId = SpeedtestManager::GetInstance().RegisterPacket(m_flowId);
            entry = Create<SpeedtestEntry>();
            m_trace_map[packetId] = entry;
        } else {
            packetId = m_reusePacketId;
            m_reusePacketId = 0;
            entry = m_trace_map[packetId];
        }
        SpeedtestTag tag(packetId);
        packet->AddByteTag(tag);
        entry->sendTime = Simulator::Now().GetNanoSeconds();
        entry->socketRTT = m_current_rtt_ns;
        entry->cwnd = m_current_cwnd_byte;
        entry->progress = GetAckedBytes();

        int actual = m_socket->Send(packet);

        if (actual != -1) {
            m_txTrace(packet);
            m_totBytes += actual;
            m_lastSendTime = Simulator::Now();
        }

        if (actual == -1) {
            m_reusePacketId = packetId;
            break;
        }

        if (actual != (int) m_packetSize) {
            NS_ABORT_MSG("Packet was not fully transmitted, this should not happen.");
        }
    }
}

void TCPSpeedtestSender::SendDataForced(bool force_packet_creation) {
    NS_LOG_FUNCTION(this);

    if (m_cancel) return;

    Ptr<Packet> packet;
    bool was_pending = false;
    if (force_packet_creation) {
        packet = Create<Packet>(m_packetSize);

        SpeedtestTag tag(SpeedtestManager::GetInstance().RegisterPacket(m_flowId));
        packet->AddByteTag(tag);
        Ptr<SpeedtestEntry> entry = Create<SpeedtestEntry>();
        entry->socketRTT = m_current_rtt_ns;
        entry->cwnd = m_current_cwnd_byte;
        entry->progress = GetAckedBytes();
        m_trace_map[tag.GetId()] = entry;
    } else {
        if (m_pending.empty()) {
            NS_ABORT_MSG("Packet should be in Queue, but isnt.");
            return;
        }

        was_pending = true;
        packet = m_pending.front();
        m_pending.pop_front();
    }

    int actual = m_socket->Send(packet);
    if (actual != -1) {
        m_txTrace(packet);
        m_totBytes += actual;
    }

    if (actual != (int) m_packetSize) {
        if (was_pending) {
            m_pending.push_front(packet);
        } else {
            m_pending.push_back(packet);
        }
    }

    // Try to keep up with the planned rate.
    m_lastSendTime = Simulator::Now();
    if (force_packet_creation) {
        ScheduleNextTx();
    }
}

void TCPSpeedtestSender::SendData(void) {
    NS_LOG_FUNCTION(this);
    SendDataForced(true);
}

void TCPSpeedtestSender::ConnectionSucceeded(Ptr <Socket> socket) {
    NS_LOG_FUNCTION(this << socket);
    NS_LOG_LOGIC("TCPSpeedtestSender Connection succeeded");
    m_connected = true;
    if (m_noLimit) {
        SendDataForcedNoLimit();
    } else {
        ScheduleNextTx();
    }
}

void TCPSpeedtestSender::ConnectionFailed(Ptr <Socket> socket) {
    NS_LOG_FUNCTION(this << socket);
    NS_LOG_LOGIC("TCPSpeedtestSender, Connection Failed");
    m_completionTimeNs = Simulator::Now().GetNanoSeconds();
    m_connFailed = true;
    m_closedByError = false;
    m_closedNormally = false;
    m_ackedBytes = 0;
    m_isCompleted = false;
    m_socket = 0;
}

void TCPSpeedtestSender::DataSend(Ptr <Socket>, uint32_t) {
    NS_LOG_FUNCTION(this);
    if (m_noLimit) {
        SendDataForcedNoLimit();
        return;
    }

    if (m_connected && !m_pending.empty()) {
        std::cout << "Retry to send a packet that was delayed due to full buffer!" << std::endl;
        SendDataForced(false);
    }
}

void TCPSpeedtestSender::SocketClosedNormal(Ptr <Socket> socket) {
    std::cout << "Speedtest@" << m_node->GetId() << ": Socket closed without error!" << std::endl;

    m_completionTimeNs = Simulator::Now().GetNanoSeconds();
    m_connFailed = false;
    m_closedByError = false;
    m_closedNormally = true;
    if (m_socket->GetObject<TcpSocketBase>()->GetTxBuffer()->Size() != 0) {
        throw std::runtime_error("Socket closed normally but send buffer is not empty");
    }
    m_ackedBytes = m_totBytes - m_socket->GetObject<TcpSocketBase>()->GetTxBuffer()->Size();
    m_isCompleted = true;
    m_socket = 0;
    m_cancel = true;
}

void TCPSpeedtestSender::SocketClosedError(Ptr <Socket> socket) {
    std::cout << "Speedtest@" << m_node->GetId() << ": Socket closed with error!" << std::endl;
    m_completionTimeNs = Simulator::Now().GetNanoSeconds();
    m_connFailed = false;
    m_closedByError = true;
    m_closedNormally = false;
    m_ackedBytes = m_totBytes - m_socket->GetObject<TcpSocketBase>()->GetTxBuffer()->Size();
    m_isCompleted = false;
    m_socket = 0;
    m_cancel = true;
}

int64_t TCPSpeedtestSender::GetAckedBytes() {
    if (m_connFailed || m_closedNormally || m_closedByError) {
        return m_ackedBytes;
    } else {
        return m_totBytes - m_socket->GetObject<TcpSocketBase>()->GetTxBuffer()->Size();
    }
}

int64_t TCPSpeedtestSender::GetCompletionTimeNs() {
    return m_completionTimeNs;
}

bool TCPSpeedtestSender::IsCompleted() {
    return m_isCompleted;
}

bool TCPSpeedtestSender::IsConnFailed() {
    return m_connFailed;
}

bool TCPSpeedtestSender::IsClosedNormally() {
    return m_closedNormally;
}

bool TCPSpeedtestSender::IsClosedByError() {
    return m_closedByError;
}

void
TCPSpeedtestSender::CwndChange(uint32_t oldCwnd, uint32_t newCwnd)
{
    m_current_cwnd_byte = newCwnd;
}

void
TCPSpeedtestSender::RttChange (Time oldRtt, Time newRtt)
{
    m_current_rtt_ns = newRtt.GetNanoSeconds();
}

uint64_t TCPSpeedtestSender::getPacketId(Ptr<const Packet> packet) {
  SpeedtestTag tag;
  if (!packet->FindFirstMatchingByteTag(tag)) {
    NS_LOG_ERROR ("Unable to get packet tag of packet " << packet->GetUid() << "!");
    return 0;
  }

  return tag.GetId();
}

void TCPSpeedtestSender::addReceptionDetails(Ptr<const Packet> packet, uint32_t at_node) {
    if (!m_trackAtDev && at_node != m_receiverId) return;

    SpeedtestTag tag;
    if (!packet->FindFirstMatchingByteTag(tag)) return;
    Ptr<SpeedtestEntry> element = m_trace_map.at(tag.GetId());
    element->receiveTime = Simulator::Now().GetNanoSeconds();
}

void TCPSpeedtestSender::addTransmissionDetails(Ptr<const Packet> packet, uint32_t at_node) {
    if (!m_trackAtDev) return;

    SpeedtestTag tag;
    if (!packet->FindFirstMatchingByteTag(tag)) return;
    Ptr<SpeedtestEntry> element = m_trace_map.at(tag.GetId());
    element->sendTime = Simulator::Now().GetNanoSeconds();
}

void TCPSpeedtestSender::WriteResults() {
    std::ostringstream oss;
    oss << "speedtest_" << m_node->GetId() << ".csv";
    std::string filename = oss.str();

    NS_LOG_INFO ("Writing speedtest file to " << filename << " ... ");
    FILE *test_csv = fopen(filename.c_str(), "w+");

    fprintf(test_csv, "send_time,receive_time,sock_rtt,sock_cwnd,progress\n");

    for (const auto& map_entry : m_trace_map) {
        const auto& entry = map_entry.second;
        fprintf(test_csv, "%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRId64 ",%" PRId64 "\n",
            entry->sendTime,
            entry->receiveTime,
            entry->socketRTT,
            entry->cwnd,
            entry->progress
        );
    }

    fclose(test_csv);
    NS_LOG_INFO ("Closed speedtest file " << filename);
}

bool SpeedtestManager::isPacketFrom(Ptr<Packet> pkt, uint64_t nodeId) {
    TCPSpeedtestSender *sender = GetSender(pkt);
    if (!sender) return false;

    return nodeId == sender->GetNode()->GetId();
}

bool SpeedtestManager::isPacketTo(Ptr<Packet> pkt, uint64_t nodeId) {
    TCPSpeedtestSender *sender = GetSender(pkt);
    if (!sender) return false;

    return nodeId == sender->m_receiverId;
}

} // Namespace ns3
