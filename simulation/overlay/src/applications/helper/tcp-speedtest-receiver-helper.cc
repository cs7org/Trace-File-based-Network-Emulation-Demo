#include "tcp-speedtest-receiver-helper.h"

#include "ns3/string.h"
#include "ns3/inet-socket-address.h"
#include "ns3/names.h"
#include "ns3/boolean.h"

namespace ns3 {

TCPSpeedtestReceiverHelper::TCPSpeedtestReceiverHelper (Address address)
{
  m_factory.SetTypeId ("ns3::TCPSpeedtestReceiver");
  SetAttribute ("Local", AddressValue (address));
}

void 
TCPSpeedtestReceiverHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
TCPSpeedtestReceiverHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
TCPSpeedtestReceiverHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
TCPSpeedtestReceiverHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<Application> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
