#include "tcp-speedtest-receiver.h"

#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/tcp-speedtest-sender.h"
#include "ns3/boolean.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TCPSpeedtestReceiver");

NS_OBJECT_ENSURE_REGISTERED (TCPSpeedtestReceiver);

TypeId
TCPSpeedtestReceiver::GetTypeId(void) {
    static TypeId tid = TypeId("ns3::TCPSpeedtestReceiver")
            .SetParent<Application>()
            .SetGroupName("Applications")
            .AddConstructor<TCPSpeedtestReceiver>()
            .AddAttribute("Local",
                          "The Address on which to Bind the rx socket.",
                          AddressValue(),
                          MakeAddressAccessor(&TCPSpeedtestReceiver::m_local),
                          MakeAddressChecker())
            .AddAttribute("Protocol",
                          "The type id of the protocol to use for the rx socket.",
                          TypeIdValue(TcpSocketFactory::GetTypeId()),
                          MakeTypeIdAccessor(&TCPSpeedtestReceiver::m_tid),
                          MakeTypeIdChecker());
    return tid;
}

TCPSpeedtestReceiver::TCPSpeedtestReceiver() {
    NS_LOG_FUNCTION(this);
    m_socket = 0;
    m_totalRx = 0;
}

TCPSpeedtestReceiver::~TCPSpeedtestReceiver() {
    NS_LOG_FUNCTION(this);
}

void TCPSpeedtestReceiver::DoDispose(void) {
    NS_LOG_FUNCTION(this);
    m_socket = 0;
    m_socketList.clear();

    // chain up
    Application::DoDispose();
}


void TCPSpeedtestReceiver::StartApplication() {
    NS_LOG_FUNCTION(this);

    if (!m_socket) {
        m_socket = Socket::CreateSocket(GetNode(), m_tid);
        if (m_socket->Bind(m_local) == -1) {
            NS_FATAL_ERROR("Failed to bind socket");
        }
        m_socket->Listen();
        m_socket->ShutdownSend();
        if (addressUtils::IsMulticast(m_local)) {
            throw std::runtime_error("No support for UDP here");
        }
    }

    m_socket->SetRecvCallback(MakeCallback(&TCPSpeedtestReceiver::HandleRead, this));
    m_socket->SetAcceptCallback(
            MakeNullCallback<bool, Ptr<Socket>,const Address &>(),
            MakeCallback(&TCPSpeedtestReceiver::HandleAccept, this)
    );

}

void TCPSpeedtestReceiver::StopApplication() {
    NS_LOG_FUNCTION(this);
    while (!m_socketList.empty()) {
        Ptr <Socket> socket = m_socketList.front();
        m_socketList.pop_front();
        socket->Close();
    }
    if (m_socket) {
        m_socket->Close();
    }
}

void TCPSpeedtestReceiver::HandleAccept(Ptr<Socket> socket, const Address &from) {
    NS_LOG_FUNCTION(this << socket << from);
    socket->SetRecvCallback (MakeCallback (&TCPSpeedtestReceiver::HandleRead, this));
    socket->SetCloseCallbacks(
            MakeCallback(&TCPSpeedtestReceiver::HandlePeerClose, this),
            MakeCallback(&TCPSpeedtestReceiver::HandlePeerError, this)
    );
    m_socketList.push_back(socket);
}

void TCPSpeedtestReceiver::HandleRead(Ptr<Socket> socket) {
    NS_LOG_FUNCTION (this << socket);

    Ptr<Packet> packet;
    Address from;
    while ((packet = socket->RecvFrom(from))) {
        if (packet->GetSize() == 0) { // EOFs
            break;
        }
        m_totalRx += packet->GetSize ();

        TCPSpeedtestSender *sender = SpeedtestManager::GetInstance().GetSender(packet);
        if (!sender) {
            SpeedtestManager::GetInstance().DebugPacket(packet);
            NS_FATAL_ERROR("Unmapped speedtest packet received!");
        } else if (!sender->m_trackAtDev) {
            sender->addReceptionDetails(packet, GetNode()->GetId());
        }
    }
}

void TCPSpeedtestReceiver::HandlePeerClose(Ptr<Socket> socket) {
    NS_LOG_FUNCTION(this << socket);
    CleanUp(socket);
}

void TCPSpeedtestReceiver::HandlePeerError(Ptr<Socket> socket) {
    NS_LOG_FUNCTION(this << socket);
    CleanUp(socket);
}

void TCPSpeedtestReceiver::CleanUp(Ptr<Socket> socket) {
    NS_LOG_FUNCTION(this << socket);
    std::list<Ptr<Socket>>::iterator it;
    for (it = m_socketList.begin(); it != m_socketList.end(); ++it) {
        if (*it == socket) {
            m_socketList.erase(it);
            break;
        }
    }
}

} // Namespace ns3
