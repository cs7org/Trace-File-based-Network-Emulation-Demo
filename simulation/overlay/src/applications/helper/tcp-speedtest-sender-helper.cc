
#include "tcp-speedtest-sender-helper.h"

#include "ns3/inet-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/names.h"
#include "ns3/data-rate.h"

namespace ns3 {

TCPSpeedtestSenderHelper::TCPSpeedtestSenderHelper (Address address, uint32_t receiverId)
{
  m_factory.SetTypeId ("ns3::TCPSpeedtestSender");
  m_factory.Set ("Remote", AddressValue (address));
  m_factory.Set ("ReceiverID", UintegerValue(receiverId));
}

void 
TCPSpeedtestSenderHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
TCPSpeedtestSenderHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

Ptr<Application>
TCPSpeedtestSenderHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<Application> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
