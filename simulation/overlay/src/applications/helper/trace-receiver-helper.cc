#include "trace-receiver-helper.h"

#include "ns3/string.h"
#include "ns3/inet-socket-address.h"
#include "ns3/names.h"

namespace ns3 {

TraceReceiverHelper::TraceReceiverHelper (Address address)
{
  m_factory.SetTypeId ("ns3::TraceReceiver");
  m_factory.Set ("Local", AddressValue (address));
}

void 
TraceReceiverHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
TraceReceiverHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
TraceReceiverHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
TraceReceiverHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
TraceReceiverHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<Application> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
