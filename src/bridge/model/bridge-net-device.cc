/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gustavo Carneiro  <gjc@inescporto.pt>
 */
#include "bridge-net-device.h"

#include "ns3/boolean.h"
#include "ns3/channel.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"

/**
 * @file
 * @ingroup bridge
 * ns3::BridgeNetDevice implementation.
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("BridgeNetDevice");

NS_OBJECT_ENSURE_REGISTERED(BridgeNetDevice);

TypeId
BridgeNetDevice::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::BridgeNetDevice")
            .SetParent<NetDevice>()
            .SetGroupName("Bridge")
            .AddConstructor<BridgeNetDevice>()
            .AddAttribute("Mtu",
                          "The MAC-level Maximum Transmission Unit",
                          UintegerValue(1500),
                          MakeUintegerAccessor(&BridgeNetDevice::SetMtu, &BridgeNetDevice::GetMtu),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("EnableLearning",
                          "Enable the learning mode of the Learning Bridge",
                          BooleanValue(true),
                          MakeBooleanAccessor(&BridgeNetDevice::m_enableLearning),
                          MakeBooleanChecker())
            .AddAttribute("ExpirationTime",
                          "Time it takes for learned MAC state entry to expire.",
                          TimeValue(Seconds(300)),
                          MakeTimeAccessor(&BridgeNetDevice::m_expirationTime),
                          MakeTimeChecker());
    return tid;
}

BridgeNetDevice::BridgeNetDevice()
    : m_node(nullptr),
      m_ifIndex(0)
{
    NS_LOG_FUNCTION_NOARGS();
    m_channel = CreateObject<BridgeChannel>();
}

BridgeNetDevice::~BridgeNetDevice()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
BridgeNetDevice::DoDispose()
{
    NS_LOG_FUNCTION_NOARGS();
    for (auto iter = m_ports.begin(); iter != m_ports.end(); iter++)
    {
        *iter = nullptr;
    }
    m_ports.clear();
    m_channel = nullptr;
    m_node = nullptr;
    NetDevice::DoDispose();
}

void
BridgeNetDevice::ReceiveFromDevice(Ptr<NetDevice> incomingPort,
                                   Ptr<const Packet> packet,
                                   uint16_t protocol,
                                   const Address& src,
                                   const Address& dst,
                                   PacketType packetType)
{
    NS_LOG_FUNCTION_NOARGS();
    NS_LOG_DEBUG("UID is " << packet->GetUid());

    Mac48Address src48 = Mac48Address::ConvertFrom(src);
    Mac48Address dst48 = Mac48Address::ConvertFrom(dst);

    if (!m_promiscRxCallback.IsNull())
    {
        m_promiscRxCallback(this, packet, protocol, src, dst, packetType);
    }

    switch (packetType)
    {
    case PACKET_HOST:
        if (dst48 == m_address)
        {
            Learn(src48, incomingPort);
            m_rxCallback(this, packet, protocol, src);
        }
        break;

    case PACKET_BROADCAST:
    case PACKET_MULTICAST:
        m_rxCallback(this, packet, protocol, src);
        ForwardBroadcast(incomingPort, packet, protocol, src48, dst48);
        break;

    case PACKET_OTHERHOST:
        if (dst48 == m_address)
        {
            Learn(src48, incomingPort);
            m_rxCallback(this, packet, protocol, src);
        }
        else
        {
            ForwardUnicast(incomingPort, packet, protocol, src48, dst48);
        }
        break;
    }
}

void
BridgeNetDevice::ForwardUnicast(Ptr<NetDevice> incomingPort,
                                Ptr<const Packet> packet,
                                uint16_t protocol,
                                Mac48Address src,
                                Mac48Address dst)
{
    NS_LOG_FUNCTION_NOARGS();
    NS_LOG_DEBUG("LearningBridgeForward (incomingPort="
                 << incomingPort->GetInstanceTypeId().GetName() << ", packet=" << packet
                 << ", protocol=" << protocol << ", src=" << src << ", dst=" << dst << ")");

    Learn(src, incomingPort);
    Ptr<NetDevice> outPort = GetLearnedState(dst);
    if (outPort && outPort != incomingPort)
    {
        NS_LOG_LOGIC("Learning bridge state says to use port `"
                     << outPort->GetInstanceTypeId().GetName() << "'");
        outPort->SendFrom(packet->Copy(), src, dst, protocol);
    }
    else
    {
        NS_LOG_LOGIC("No learned state: send through all ports");
        for (auto iter = m_ports.begin(); iter != m_ports.end(); iter++)
        {
            Ptr<NetDevice> port = *iter;
            if (port != incomingPort)
            {
                NS_LOG_LOGIC("LearningBridgeForward ("
                             << src << " => " << dst
                             << "): " << incomingPort->GetInstanceTypeId().GetName() << " --> "
                             << port->GetInstanceTypeId().GetName() << " (UID " << packet->GetUid()
                             << ").");
                port->SendFrom(packet->Copy(), src, dst, protocol);
            }
        }
    }
}

void
BridgeNetDevice::ForwardBroadcast(Ptr<NetDevice> incomingPort,
                                  Ptr<const Packet> packet,
                                  uint16_t protocol,
                                  Mac48Address src,
                                  Mac48Address dst)
{
    NS_LOG_FUNCTION_NOARGS();
    NS_LOG_DEBUG("LearningBridgeForward (incomingPort="
                 << incomingPort->GetInstanceTypeId().GetName() << ", packet=" << packet
                 << ", protocol=" << protocol << ", src=" << src << ", dst=" << dst << ")");
    Learn(src, incomingPort);

    for (auto iter = m_ports.begin(); iter != m_ports.end(); iter++)
    {
        Ptr<NetDevice> port = *iter;
        if (port != incomingPort)
        {
            NS_LOG_LOGIC("LearningBridgeForward (" << src << " => " << dst << "): "
                                                   << incomingPort->GetInstanceTypeId().GetName()
                                                   << " --> " << port->GetInstanceTypeId().GetName()
                                                   << " (UID " << packet->GetUid() << ").");
            port->SendFrom(packet->Copy(), src, dst, protocol);
        }
    }
}

void
BridgeNetDevice::Learn(Mac48Address source, Ptr<NetDevice> port)
{
    NS_LOG_FUNCTION_NOARGS();
    if (m_enableLearning)
    {
        LearnedState& state = m_learnState[source];
        state.associatedPort = port;
        state.expirationTime = Simulator::Now() + m_expirationTime;
    }
}

Ptr<NetDevice>
BridgeNetDevice::GetLearnedState(Mac48Address source)
{
    NS_LOG_FUNCTION_NOARGS();
    if (m_enableLearning)
    {
        Time now = Simulator::Now();
        auto iter = m_learnState.find(source);
        if (iter != m_learnState.end())
        {
            LearnedState& state = iter->second;
            if (state.expirationTime > now)
            {
                return state.associatedPort;
            }
            else
            {
                m_learnState.erase(iter);
            }
        }
    }
    return nullptr;
}

uint32_t
BridgeNetDevice::GetNBridgePorts() const
{
    NS_LOG_FUNCTION_NOARGS();
    return m_ports.size();
}

Ptr<NetDevice>
BridgeNetDevice::GetBridgePort(uint32_t n) const
{
    NS_LOG_FUNCTION_NOARGS();
    return m_ports[n];
}

void
BridgeNetDevice::AddBridgePort(Ptr<NetDevice> bridgePort)
{
    NS_LOG_FUNCTION_NOARGS();
    NS_ASSERT(bridgePort != this);
    if (!Mac48Address::IsMatchingType(bridgePort->GetAddress()))
    {
        NS_FATAL_ERROR("Device does not support eui 48 addresses: cannot be added to bridge.");
    }
    if (!bridgePort->SupportsSendFrom())
    {
        NS_FATAL_ERROR("Device does not support SendFrom: cannot be added to bridge.");
    }
    if (m_address == Mac48Address())
    {
        m_address = Mac48Address::ConvertFrom(bridgePort->GetAddress());
    }

    NS_LOG_DEBUG("RegisterProtocolHandler for " << bridgePort->GetInstanceTypeId().GetName());
    m_node->RegisterProtocolHandler(MakeCallback(&BridgeNetDevice::ReceiveFromDevice, this),
                                    0,
                                    bridgePort,
                                    true);
    m_ports.push_back(bridgePort);
    m_channel->AddChannel(bridgePort->GetChannel());
}

void
BridgeNetDevice::SetIfIndex(const uint32_t index)
{
    NS_LOG_FUNCTION_NOARGS();
    m_ifIndex = index;
}

uint32_t
BridgeNetDevice::GetIfIndex() const
{
    NS_LOG_FUNCTION_NOARGS();
    return m_ifIndex;
}

Ptr<Channel>
BridgeNetDevice::GetChannel() const
{
    NS_LOG_FUNCTION_NOARGS();
    return m_channel;
}

void
BridgeNetDevice::SetAddress(Address address)
{
    NS_LOG_FUNCTION_NOARGS();
    m_address = Mac48Address::ConvertFrom(address);
}

Address
BridgeNetDevice::GetAddress() const
{
    NS_LOG_FUNCTION_NOARGS();
    return m_address;
}

bool
BridgeNetDevice::SetMtu(const uint16_t mtu)
{
    NS_LOG_FUNCTION_NOARGS();
    m_mtu = mtu;
    return true;
}

uint16_t
BridgeNetDevice::GetMtu() const
{
    NS_LOG_FUNCTION_NOARGS();
    return m_mtu;
}

bool
BridgeNetDevice::IsLinkUp() const
{
    NS_LOG_FUNCTION_NOARGS();
    return true;
}

void
BridgeNetDevice::AddLinkChangeCallback(Callback<void> callback)
{
}

bool
BridgeNetDevice::IsBroadcast() const
{
    NS_LOG_FUNCTION_NOARGS();
    return true;
}

Address
BridgeNetDevice::GetBroadcast() const
{
    NS_LOG_FUNCTION_NOARGS();
    return Mac48Address::GetBroadcast();
}

bool
BridgeNetDevice::IsMulticast() const
{
    NS_LOG_FUNCTION_NOARGS();
    return true;
}

Address
BridgeNetDevice::GetMulticast(Ipv4Address multicastGroup) const
{
    NS_LOG_FUNCTION(this << multicastGroup);
    Mac48Address multicast = Mac48Address::GetMulticast(multicastGroup);
    return multicast;
}

bool
BridgeNetDevice::IsPointToPoint() const
{
    NS_LOG_FUNCTION_NOARGS();
    return false;
}

bool
BridgeNetDevice::IsBridge() const
{
    NS_LOG_FUNCTION_NOARGS();
    return true;
}

bool
BridgeNetDevice::Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
    NS_LOG_FUNCTION_NOARGS();
    return SendFrom(packet, m_address, dest, protocolNumber);
}

bool
BridgeNetDevice::SendFrom(Ptr<Packet> packet,
                          const Address& src,
                          const Address& dest,
                          uint16_t protocolNumber)
{
    NS_LOG_FUNCTION_NOARGS();
    Mac48Address dst = Mac48Address::ConvertFrom(dest);

    // try to use the learned state if data is unicast
    if (!dst.IsGroup())
    {
        Ptr<NetDevice> outPort = GetLearnedState(dst);
        if (outPort)
        {
            outPort->SendFrom(packet, src, dest, protocolNumber);
            return true;
        }
    }

    // data was not unicast or no state has been learned for that mac
    // address => flood through all ports.
    Ptr<Packet> pktCopy;
    for (auto iter = m_ports.begin(); iter != m_ports.end(); iter++)
    {
        pktCopy = packet->Copy();
        Ptr<NetDevice> port = *iter;
        port->SendFrom(pktCopy, src, dest, protocolNumber);
    }

    return true;
}

Ptr<Node>
BridgeNetDevice::GetNode() const
{
    NS_LOG_FUNCTION_NOARGS();
    return m_node;
}

void
BridgeNetDevice::SetNode(Ptr<Node> node)
{
    NS_LOG_FUNCTION_NOARGS();
    m_node = node;
}

bool
BridgeNetDevice::NeedsArp() const
{
    NS_LOG_FUNCTION_NOARGS();
    return true;
}

void
BridgeNetDevice::SetReceiveCallback(NetDevice::ReceiveCallback cb)
{
    NS_LOG_FUNCTION_NOARGS();
    m_rxCallback = cb;
}

void
BridgeNetDevice::SetPromiscReceiveCallback(NetDevice::PromiscReceiveCallback cb)
{
    NS_LOG_FUNCTION_NOARGS();
    m_promiscRxCallback = cb;
}

bool
BridgeNetDevice::SupportsSendFrom() const
{
    NS_LOG_FUNCTION_NOARGS();
    return true;
}

Address
BridgeNetDevice::GetMulticast(Ipv6Address addr) const
{
    NS_LOG_FUNCTION(this << addr);
    return Mac48Address::GetMulticast(addr);
}

} // namespace ns3
