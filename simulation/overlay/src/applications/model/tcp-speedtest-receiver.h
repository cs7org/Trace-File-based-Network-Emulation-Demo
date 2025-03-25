#ifndef TCP_SPEEDTEST_RECEIVER_H
#define TCP_SPEEDTEST_RECEIVER_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"

namespace ns3 {

class Address;
class Socket;
class Packet;

class TCPSpeedtestReceiver : public Application
{
public:
  static TypeId GetTypeId (void);
  TCPSpeedtestReceiver ();
  virtual ~TCPSpeedtestReceiver ();

protected:
  virtual void DoDispose (void);

private:
    
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop
  
  void HandleRead (Ptr<Socket> socket);
  void HandleAccept (Ptr<Socket> socket, const Address& from);
  void HandlePeerClose (Ptr<Socket> socket);
  void HandlePeerError (Ptr<Socket> socket);
  void CleanUp (Ptr<Socket> socket);

  Ptr<Socket> m_socket;                 //!< Listening socket
  std::list<Ptr<Socket> > m_socketList; //!< the accepted sockets

  Address m_local;        //!< Local address to bind to
  TypeId  m_tid;          //!< Protocol TypeId
  uint64_t m_totalRx;     //!< Total bytes received
};

} // namespace ns3

#endif /* TCP_SPEEDTEST_RECEIVER_H */
