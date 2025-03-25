#ifndef TCP_SPEEDTEST_SENDER_H
#define TCP_SPEEDTEST_SENDER_H

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/string.h"
#include "ns3/traced-callback.h"
#include "ns3/data-rate.h"

#include <map>
#include <queue>

namespace ns3 {

class Address;
class Socket;

class SpeedtestEntry : public Object {
  public:
    uint64_t sendTime;
    uint64_t receiveTime;
    uint64_t socketRTT;
    int64_t progress;
    int64_t cwnd;

    SpeedtestEntry();
};

class SpeedtestTag : public ns3::Tag {
public:
  SpeedtestTag() : m_id(0) {}
  explicit SpeedtestTag(uint64_t id) : m_id(id) {}
  
  void Serialize(TagBuffer i) const override { i.WriteU64(m_id); }
  void Deserialize(TagBuffer i) override { m_id = i.ReadU64(); }
  uint32_t GetSerializedSize() const override { return sizeof(m_id); }
  void Print(std::ostream &os) const override { os << "PacketId=" << m_id; }
  
  void SetId(uint64_t id) { m_id = id; }
  uint64_t GetId() const { return m_id; }
  
  static ns3::TypeId GetTypeId() {
    static ns3::TypeId tid = ns3::TypeId("SpeedtestTag")
        .SetParent<ns3::Tag>()
        .SetGroupName("Tracer");
    return tid;
  }
  
  virtual ns3::TypeId GetInstanceTypeId() const override {
      return GetTypeId();
  }
  
  void SetPacketTag(Ptr<Packet> packet) {
    packet->AddByteTag(*this);
  }
  
  bool GetPacketTag(Ptr<Packet> packet) {
    return packet->FindFirstMatchingByteTag(*this);
  }
  
private:
  uint64_t m_id;
};

class TCPSpeedtestSender : public Application
{
public:
  static TypeId GetTypeId (void);

  TCPSpeedtestSender ();

  virtual ~TCPSpeedtestSender ();

  int64_t GetAckedBytes();
  int64_t GetCompletionTimeNs();
  bool IsCompleted();
  bool IsConnFailed();
  bool IsClosedByError();
  bool IsClosedNormally();
  void WriteResults();

  uint64_t getPacketId(Ptr<const Packet> packet);
  void addReceptionDetails(Ptr<const Packet> packet, uint32_t at_node);
  void addTransmissionDetails(Ptr<const Packet> packet, uint32_t at_node);

  uint64_t        m_receiverId;
  bool            m_trackAtDev;

protected:
  virtual void DoDispose (void);
private:
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop

  std::map<uint64_t, Ptr<SpeedtestEntry>> m_trace_map{};

  /**
   * Send data until the L4 transmission buffer is full.
   */
  void SendData ();
  void SendDataForced (bool);
  void SendDataForcedNoLimit ();

  Ptr<Socket>     m_socket;       //!< Associated socket
  Address         m_peer;         //!< Peer address
  bool            m_connected;    //!< True if connected
  DataRate        m_dataRate;     //!< Transmit data rate
  uint32_t        m_packetSize;   //!< Transmit packet size
  uint64_t        m_totBytes;     //!< Total bytes sent so far
  TypeId          m_tid;          //!< The type of protocol to use.
  int64_t         m_completionTimeNs; //!< Completion time in nanoseconds
  bool            m_connFailed;       //!< Whether the connection failed
  bool            m_closedNormally;   //!< Whether the connection closed normally
  bool            m_closedByError;    //!< Whether the connection closed by error
  uint64_t        m_ackedBytes;       //!< Amount of acknowledged bytes cached after close of the socket
  bool            m_isCompleted;      //!< True iff the flow is completed fully AND closed normally
  uint32_t        m_current_cwnd_byte;     //!< Current congestion window (detailed logging)
  int64_t         m_current_rtt_ns;        //!< Current last RTT sample (detailed logging)
  Time            m_lastSendTime;     //!< Last send timestamp
  uint64_t        m_flowId;
  bool            m_noLimit;
  bool            m_cancel;
  uint64_t        m_reusePacketId;
  std::deque<Ptr<Packet>>     m_pending;

  // TCP flow logging
  TracedCallback<Ptr<const Packet> > m_txTrace;

private:
  void ConnectionSucceeded (Ptr<Socket> socket);
  void ConnectionFailed (Ptr<Socket> socket);
  void DataSend (Ptr<Socket>, uint32_t);
  void SocketClosedNormal(Ptr<Socket> socket);
  void SocketClosedError(Ptr<Socket> socket);
  void CwndChange(uint32_t, uint32_t newCwnd);
  void RttChange (Time, Time newRtt);
  void ScheduleNextTx();

};

class SpeedtestManager {
  public:
  static SpeedtestManager& GetInstance() {
    static SpeedtestManager instance;
    return instance;
  }

  uint16_t RegisterSender(TCPSpeedtestSender *sender) {
    uint16_t id = flow_counter++;
    flow_map[id] = sender;
    return id;
  }

  uint64_t RegisterPacket(uint16_t src) {
    uint64_t packet = packet_counter++;
    packet_map[packet] = src;
    return packet;
  }

  TCPSpeedtestSender *PacketToSender(uint64_t packet_id) {
    if (packet_map.count(packet_id) == 0) {
      return 0;
    }

    uint16_t handler = packet_map.at(packet_id);

    if (flow_map.count(handler) == 0) {
      return 0;
    }

    return flow_map.at(handler);
  }

  bool isPacketFrom(Ptr<Packet> pkt, uint64_t nodeId);
  bool isPacketTo(Ptr<Packet> pkt, uint64_t nodeId);

  TCPSpeedtestSender *GetSender(Ptr<const Packet> packet);
  void DebugPacket(Ptr<const Packet> packet);
private:
  SpeedtestManager() {}
  SpeedtestManager(const SpeedtestManager&) = delete;
  SpeedtestManager& operator=(const SpeedtestManager) = delete;

  uint16_t flow_counter = 1;
  uint64_t packet_counter = 1;
  std::map<uint16_t, TCPSpeedtestSender *> flow_map{};
  std::map<uint64_t, uint16_t> packet_map{};
};

} // namespace ns3

#endif /* TCP_SPEEDTEST_SENDER_H */
