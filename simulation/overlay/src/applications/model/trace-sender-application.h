#ifndef TRACE_SENDER_H
#define TRACE_SENDER_H

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/data-rate.h"
#include "ns3/traced-callback.h"
#include "ns3/seq-ts-size-header.h"

#include <map>
#include <tuple>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <functional>

namespace ns3 {

class Address;
class RandomVariableStream;
class Socket;

class TraceElement : public Object {
public:
  uint64_t send_time;
  uint64_t receive_time ;
  std::vector<double> link_capacities;
  std::vector<uint64_t> queue_capacities;
  std::vector<uint64_t> transmit_times;
  std::vector<uint64_t> path;
  uint16_t path_id;
  bool received;
  uint32_t hop_count;

  TraceElement();
};

class TracePacketTag : public ns3::Tag {
public:
  TracePacketTag() : m_id(0) {}
  explicit TracePacketTag(uint64_t id) : m_id(id) {}

  void Serialize(TagBuffer i) const override { i.WriteU64(m_id); }
  void Deserialize(TagBuffer i) override { m_id = i.ReadU64(); }
  uint32_t GetSerializedSize() const override { return sizeof(m_id); }
  void Print(std::ostream &os) const override { os << "PacketId=" << m_id; }

  void SetId(uint64_t id) { m_id = id; }
  uint64_t GetId() const { return m_id; }

  static ns3::TypeId GetTypeId() {
    static ns3::TypeId tid = ns3::TypeId("TracePacketTag")
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

enum class QueueAccumulationMethod {
  SUM,
  BOTTLENECK,
  SUM_TO_BOTTLENECK
};

class TraceSender : public Application 
{
public:
  static TypeId GetTypeId (void);
  TraceSender ();
  virtual ~TraceSender();
  Ptr<Socket> GetSocket (void) const;
  int64_t AssignStreams (int64_t stream);

  bool isTracePacket(Ptr<const Packet> packet);
  void debugTracePacket(Ptr<const Packet> packet);
  uint64_t getPacketId(Ptr<const Packet> packet);

  void addHopDetails(Ptr<const Packet> packet, double link_load, uint64_t queue_cap, uint32_t at_node);
  void addReceptionDetails(Ptr<const Packet> packet, uint32_t at_node);
  void debugTraceElement(Ptr<const Packet> packet);

  uint32_t GetSummaryIntervalNs() {
    return MilliSeconds(100).GetNanoSeconds();
  }

  void WriteResults (QueueAccumulationMethod mode);

protected:
  virtual void DoDispose (void);
private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void CancelEvents ();
  void StartSending ();
  void StopSending ();
  void SendPacket ();

  // Delay (ns), Jitter (ns), min_link_usage (bps), max_link_usage (bps), queue_cap, hops, drop_ratio 
  typedef std::tuple<uint64_t, uint64_t, double, double, uint64_t, uint32_t, double> TraceLogLine;

  std::map<uint64_t, Ptr<TraceElement>> m_trace_map{};

  uint32_t        m_fromNode;
  uint32_t        m_toNode;
  uint64_t        m_lastSummary {0};
  uint64_t        m_firstTransmission;
  bool            m_isFirstTransmission {true};

  Ptr<Socket>     m_socket;       //!< Associated socket
  Address         m_peer;         //!< Peer address
  Address         m_local;        //!< Local address to bind to
  bool            m_connected;    //!< True if connected
  uint32_t        m_interval;     //!< Transmission Interval
  uint32_t        m_summary;      //!< Summary counter
  uint32_t        m_pktSize;      //!< Size of packets
  uint64_t        m_totBytes;     //!< Total bytes sent so far
  EventId         m_startStopEvent;     //!< Event id for next start or stop event
  EventId         m_sendEvent;    //!< Event id of pending "send packet" event
  TypeId          m_tid;          //!< Type of the socket used
  uint32_t        m_seq {0};      //!< Sequence
  bool            m_enableSeqTsSizeHeader {false}; //!< Enable or disable the use of SeqTsSizeHeader
  uint16_t        m_flowId;

  TracedCallback<Ptr<const Packet> > m_txTrace;
  TracedCallback<Ptr<const Packet>, const Address &, const Address &> m_txTraceWithAddresses;
  TracedCallback<Ptr<const Packet>, const Address &, const Address &, const SeqTsSizeHeader &> m_txTraceWithSeqTsSize;

private:
  void ScheduleNextTx ();
  void ConnectionSucceeded (Ptr<Socket> socket);
  void ConnectionFailed (Ptr<Socket> socket);
};

class TraceFlowManager {
public:
  static TraceFlowManager& GetInstance() {
    static TraceFlowManager instance;
    return instance;
  }

  uint16_t RegisterTraceSender(TraceSender *sender) {
    uint16_t id = flow_counter++;
    flow_map[id] = sender;
    return id;
  }

  uint16_t RegisterTracePacket(uint16_t src) {
    uint64_t packet = packet_counter++;
    packet_map[packet] = src;
    return packet;
  }

  TraceSender *PacketToTraceSender(uint64_t packet_id) {
    if (packet_map.count(packet_id) == 0) {
      return 0;
    }

    uint16_t handler = packet_map.at(packet_id);

    if (flow_map.count(handler) == 0) {
      return 0;
    }

    return flow_map.at(handler);
  }

  TraceSender *GetTraceSender(Ptr<const Packet> packet);
  void DebugTracePacket(Ptr<const Packet> packet);
private:
  TraceFlowManager() {}
  TraceFlowManager(const TraceFlowManager&) = delete;
  TraceFlowManager& operator=(const TraceFlowManager) = delete;

  uint16_t flow_counter = 1;
  uint64_t packet_counter = 1;
  std::map<uint16_t, TraceSender *> flow_map{};
  std::map<uint64_t, uint16_t> packet_map{};
};

class TracePathManager {
public:
  static TracePathManager& GetInstance() {
    static TracePathManager instance;
    return instance;
  }

  uint16_t GetOrCreateRouteId(const std::vector<uint64_t>& path) {
    uint64_t hash = HashPath(path);

    auto it = path_to_id.find(hash);
    if (it != path_to_id.end()) {
        return it->second;
    }

    uint16_t new_id = next_id++;
    if (new_id == 0) new_id = next_id++;

    path_to_id[hash] = new_id;
    return new_id;
  }
private:
  TracePathManager() : next_id(1) {}
  TracePathManager(const TracePathManager&) = delete;
  TracePathManager& operator=(const TracePathManager) = delete;

  uint64_t HashPath(const std::vector<uint64_t>& path) const {
    std::hash<uint64_t> hasher;
    uint64_t h = 1469598103934665603ULL;
      for (auto v : path) {
        h ^= hasher(v);
        h *= 1099511628211ULL;
      }
      return h;
    }

  std::unordered_map<uint64_t, uint16_t> path_to_id;
  uint16_t next_id;
};

} // namespace ns3

#endif /* TRACE_SENDER_H */
