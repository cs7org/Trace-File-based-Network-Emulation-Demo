#ifndef TRACE_RECEIVER_HELPER_H
#define TRACE_RECEIVER_HELPER_H

#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/node-container.h"
#include "ns3/application-container.h"

namespace ns3 {

class TraceReceiverHelper
{
public:
  TraceReceiverHelper (Address address);
  void SetAttribute (std::string name, const AttributeValue &value);
  ApplicationContainer Install (NodeContainer c) const;
  ApplicationContainer Install (Ptr<Node> node) const;
  ApplicationContainer Install (std::string nodeName) const;

private:
  Ptr<Application> InstallPriv (Ptr<Node> node) const;
  ObjectFactory m_factory;
};

}

#endif /* TRACE_RECEIVER_HELPER_H */
