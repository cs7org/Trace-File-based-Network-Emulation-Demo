#ifndef TRACE_SENDER_HELPER_H
#define TRACE_SENDER_HELPER_H

#include <stdint.h>
#include <string>
#include "ns3/object-factory.h"
#include "ns3/address.h"
#include "ns3/attribute.h"
#include "ns3/net-device.h"
#include "ns3/node-container.h"
#include "ns3/application-container.h"
#include "ns3/trace-sender-application.h"

namespace ns3 {

class DataRate;

class TraceSenderHelper
{
public:
  TraceSenderHelper (Address address, uint32_t from_node, uint32_t to_node);
  void SetAttribute (std::string name, const AttributeValue &value);
  void SetConstantRate (DataRate dataRate, uint32_t packetSize = 512);
  ApplicationContainer Install (NodeContainer c) const;
  ApplicationContainer Install (Ptr<Node> node) const;
  ApplicationContainer Install (std::string nodeName) const;
  int64_t AssignStreams (NodeContainer c, int64_t stream);

private:
  Ptr<Application> InstallPriv (Ptr<Node> node) const;
  ObjectFactory m_factory;

};

} // namespace ns3

#endif /* TRACE_SENDER_HELPER_H */
