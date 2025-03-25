#ifndef TRACE_RECEIVER_H
#define TRACE_RECEIVER_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/seq-ts-size-header.h"
#include <unordered_map>

namespace ns3 {

class Address;
class Socket;
class Packet;

class TraceReceiver : public Application 
{
public:
  static TypeId GetTypeId (void);
  TraceReceiver ();
  virtual ~TraceReceiver ();
  uint64_t GetTotalRx () const;
  Ptr<Socket> GetListeningSocket (void) const;
  std::list<Ptr<Socket> > GetAcceptedSockets (void) const;
  typedef void (* SeqTsSizeCallback)(Ptr<const Packet> p, const Address &from, const Address & to,
                                   const SeqTsSizeHeader &header);

protected:
  virtual void DoDispose (void);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);
  void HandleRead (Ptr<Socket> socket);
  void HandleAccept (Ptr<Socket> socket, const Address& from);
  void HandlePeerClose (Ptr<Socket> socket);
  void HandlePeerError (Ptr<Socket> socket);
  void PacketReceived (const Ptr<Packet> &p, const Address &from, const Address &localAddress);

  struct AddressHash
  {
    size_t operator() (const Address &x) const
    {
      NS_ABORT_IF (!InetSocketAddress::IsMatchingType (x));
      InetSocketAddress a = InetSocketAddress::ConvertFrom (x);
      return std::hash<uint32_t>()(a.GetIpv4 ().Get ());
    }
  };

  std::unordered_map<Address, Ptr<Packet>, AddressHash> m_buffer; //!< Buffer for received packets
  Ptr<Socket>     m_socket;       //!< Listening socket
  std::list<Ptr<Socket> > m_socketList; //!< the accepted sockets
  Address         m_local;        //!< Local address to bind to
  uint64_t        m_totalRx;      //!< Total bytes received
  TypeId          m_tid;          //!< Protocol TypeId
  bool            m_enableSeqTsSizeHeader {false}; //!< Enable or disable the export of SeqTsSize header 
  TracedCallback<Ptr<const Packet>, const Address &> m_rxTrace;
  TracedCallback<Ptr<const Packet>, const Address &, const Address &> m_rxTraceWithAddresses;
  TracedCallback<Ptr<const Packet>, const Address &, const Address &, const SeqTsSizeHeader&> m_rxTraceWithSeqTsSize;
};

} // namespace ns3

#endif /* TRACE_RECEIVER_H */
