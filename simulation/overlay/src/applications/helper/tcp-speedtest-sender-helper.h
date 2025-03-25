#ifndef TCP_SPEEDTEST_SENDER_HELPER_H
#define TCP_SPEEDTEST_SENDER_HELPER_H

#include <stdint.h>
#include <string>
#include "ns3/object-factory.h"
#include "ns3/address.h"
#include "ns3/attribute.h"
#include "ns3/net-device.h"
#include "ns3/node-container.h"
#include "ns3/application-container.h"
#include "ns3/uinteger.h"

namespace ns3 {


class TCPSpeedtestSenderHelper
{
public:
  TCPSpeedtestSenderHelper (Address address, uint32_t receiverId);
  void SetAttribute (std::string name, const AttributeValue &value);
  ApplicationContainer Install (Ptr<Node> node) const;

private:
  Ptr<Application> InstallPriv (Ptr<Node> node) const;
  ObjectFactory m_factory;

};

} // namespace ns3

#endif /* TCP_SPEEDTEST_SENDER_HELPER_H */
