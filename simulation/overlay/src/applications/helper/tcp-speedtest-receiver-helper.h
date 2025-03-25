#ifndef TCP_SPEEDTEST_RECEIVER_HELPER_H
#define TCP_SPEEDTEST_RECEIVER_HELPER_H

#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/node-container.h"
#include "ns3/application-container.h"

namespace ns3 {

class TCPSpeedtestReceiverHelper
{
public:
  TCPSpeedtestReceiverHelper (Address address);
  void SetAttribute (std::string name, const AttributeValue &value);
  ApplicationContainer Install (NodeContainer c) const;
  ApplicationContainer Install (Ptr<Node> node) const;

private:
  Ptr<Application> InstallPriv (Ptr<Node> node) const;
  ObjectFactory m_factory;

};

} // namespace ns3

#endif /* TCP_SPEEDTEST_RECEIVER_HELPER_H */
