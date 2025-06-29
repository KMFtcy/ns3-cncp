#include "switch-node.h"

#include "ppp-header.h"
#include "qbb-net-device.h"

#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/flow-id-tag.h"
#include "ns3/int-header.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4.h"
#include "ns3/packet.h"
#include "ns3/pause-header.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"

#include <cmath>
#include <iomanip>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("SwitchNode");

TypeId
SwitchNode::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::SwitchNode")
                            .SetParent<Node>()
                            .AddConstructor<SwitchNode>()
                            .AddAttribute("EcnEnabled",
                                          "Enable ECN marking.",
                                          BooleanValue(false),
                                          MakeBooleanAccessor(&SwitchNode::m_ecnEnabled),
                                          MakeBooleanChecker())
                            .AddAttribute("PfcEnabled",
                                          "Enable PFC.",
                                          BooleanValue(true),
                                          MakeBooleanAccessor(&SwitchNode::m_pfcEnabled),
                                          MakeBooleanChecker())
                            .AddAttribute("CcMode",
                                          "CC mode.",
                                          UintegerValue(0),
                                          MakeUintegerAccessor(&SwitchNode::m_ccMode),
                                          MakeUintegerChecker<uint32_t>())
                            .AddAttribute("AckHighPrio",
                                          "Set high priority for ACK/NACK or not",
                                          UintegerValue(0),
                                          MakeUintegerAccessor(&SwitchNode::m_ackHighPrio),
                                          MakeUintegerChecker<uint32_t>())
                            .AddAttribute("MaxRtt",
                                          "Max Rtt of the network",
                                          UintegerValue(9000),
                                          MakeUintegerAccessor(&SwitchNode::m_maxRtt),
                                          MakeUintegerChecker<uint32_t>())
                            .AddAttribute("CNCPGamma",
                                          "CNCP Iterative Update Parameter Gamma",
                                          UintegerValue(1500),
                                          MakeUintegerAccessor(&SwitchNode::m_gamma),
                                          MakeUintegerChecker<uint64_t>())
                            .AddAttribute("CNCPLambda",
                                          "CNCP Iterative Update Parameter Lambda",
                                          UintegerValue(100000000000),
                                          MakeUintegerAccessor(&SwitchNode::m_lambda),
                                          MakeUintegerChecker<uint64_t>());
    return tid;
}

SwitchNode::SwitchNode()
{
    m_ecmpSeed = m_id;
    m_node_type = 1;
    m_mmu = CreateObject<SwitchMmu>();
    for (uint32_t i = 0; i < pCnt; i++)
    {
        for (uint32_t j = 0; j < pCnt; j++)
        {
            for (uint32_t k = 0; k < qCnt; k++)
            {
                m_bytes[i][j][k] = 0;
            }
        }
    }
    for (uint32_t i = 0; i < pCnt; i++)
    {
        m_txBytes[i] = 0;
    }
    for (uint32_t i = 0; i < pCnt; i++)
    {
        m_lastPktSize[i] = m_lastPktTs[i] = 0;
    }
    for (uint32_t i = 0; i < pCnt; i++)
    {
        m_u[i] = 0;
    }
}

int
SwitchNode::GetOutDev(Ptr<const Packet> p, CustomHeader& ch)
{
    // look up entries
    auto entry = m_rtTable.find(ch.dip);

    // no matching entry
    if (entry == m_rtTable.end())
    {
        return -1;
    }

    // entry found
    auto& nexthops = entry->second;

    // pick one next hop based on hash
    union {
        uint8_t u8[4 + 4 + 2 + 2];
        uint32_t u32[3];
    } buf;

    buf.u32[0] = ch.sip;
    buf.u32[1] = ch.dip;
    if (ch.l3Prot == 0x6)
    {
        buf.u32[2] = ch.tcp.sport | ((uint32_t)ch.tcp.dport << 16);
    }
    else if (ch.l3Prot == 0x11)
    {
        buf.u32[2] = ch.udp.sport | ((uint32_t)ch.udp.dport << 16);
    }
    else if (ch.l3Prot == 0xFC || ch.l3Prot == 0xFD)
    {
        buf.u32[2] = ch.ack.sport | ((uint32_t)ch.ack.dport << 16);
    }

    uint32_t idx = EcmpHash(buf.u8, 12, m_ecmpSeed) % nexthops.size();
    return nexthops[idx];
}

void
SwitchNode::CheckAndSendPfc(uint32_t inDev, uint32_t qIndex)
{
    Ptr<QbbNetDevice> device = DynamicCast<QbbNetDevice>(m_devices[inDev]);
    if (m_mmu->CheckShouldPause(inDev, qIndex) && m_pfcEnabled)
    {
        device->SendPfc(qIndex, 0);
        m_mmu->SetPause(inDev, qIndex);
    }
}

void
SwitchNode::CheckAndSendResume(uint32_t inDev, uint32_t qIndex)
{
    Ptr<QbbNetDevice> device = DynamicCast<QbbNetDevice>(m_devices[inDev]);
    if (m_mmu->CheckShouldResume(inDev, qIndex))
    {
        device->SendPfc(qIndex, 1);
        m_mmu->SetResume(inDev, qIndex);
    }
}

void
SwitchNode::SendToDev(Ptr<NetDevice> input_device, Ptr<Packet> p, CustomHeader& ch)
{
    int idx = GetOutDev(p, ch);
    if (idx >= 0)
    {
        NS_ASSERT_MSG(m_devices[idx]->IsLinkUp(),
                      "The routing table look up should return link that is up");

        // determine the qIndex
        uint32_t qIndex;
        if (ch.l3Prot == 0xFF || ch.l3Prot == 0xFE ||
            (m_ackHighPrio && (ch.l3Prot == 0xFD || ch.l3Prot == 0xFC)))
        { // QCN or PFC or NACK, go highest priority
            qIndex = 0;
        }
        else
        {
            qIndex = (ch.l3Prot == 0x06 ? 1 : ch.udp.pg); // if TCP, put to queue 1
        }

        // admission control
        FlowIdTag t;
        p->PeekPacketTag(t);
        uint32_t inDev = t.GetFlowId();
        if (qIndex != 0)
        {
            // notify CNCP flow control before drop, ignore ack/nack and background flow (pg=2)
            if (m_ccMode == 11 && ch.l3Prot != 0xFC && ch.l3Prot != 0xFD && ch.udp.pg != 2)
            {
                CNCPNotifyIngress(p, ch, idx, input_device);
            }
            // not highest priority
            if ((m_mmu->CheckIngressAdmission(inDev, qIndex, p->GetSize()) || !m_pfcEnabled) &&
                m_mmu->CheckEgressAdmission(idx, qIndex, p->GetSize()) &&
                CNCPAdmitIngress(p, ch))
            { // Admission control
                m_mmu->UpdateIngressAdmission(inDev, qIndex, p->GetSize());
                m_mmu->UpdateEgressAdmission(idx, qIndex, p->GetSize());
            }
            else
            {
                return; // Drop
            }
            CheckAndSendPfc(inDev, qIndex);
        }
        m_bytes[inDev][idx][qIndex] += p->GetSize();
        m_devices[idx]->SwitchSend(qIndex, p, ch);
    }
    else
    {
        return; // Drop
    }
}

uint32_t
SwitchNode::EcmpHash(const uint8_t* key, size_t len, uint32_t seed)
{
    uint32_t h = seed;
    if (len > 3)
    {
        const uint32_t* key_x4 = (const uint32_t*)key;
        size_t i = len >> 2;
        do
        {
            uint32_t k = *key_x4++;
            k *= 0xcc9e2d51;
            k = (k << 15) | (k >> 17);
            k *= 0x1b873593;
            h ^= k;
            h = (h << 13) | (h >> 19);
            h += (h << 2) + 0xe6546b64;
        } while (--i);
        key = (const uint8_t*)key_x4;
    }
    if (len & 3)
    {
        size_t i = len & 3;
        uint32_t k = 0;
        key = &key[i - 1];
        do
        {
            k <<= 8;
            k |= *key--;
        } while (--i);
        k *= 0xcc9e2d51;
        k = (k << 15) | (k >> 17);
        k *= 0x1b873593;
        h ^= k;
    }
    h ^= len;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

void
SwitchNode::SetEcmpSeed(uint32_t seed)
{
    m_ecmpSeed = seed;
}

void
SwitchNode::AddTableEntry(Ipv4Address& dstAddr, uint32_t intf_idx)
{
    uint32_t dip = dstAddr.Get();
    m_rtTable[dip].push_back(intf_idx);
}

void
SwitchNode::ClearTable()
{
    m_rtTable.clear();
}

// This function can only be called in switch mode
bool
SwitchNode::SwitchReceiveFromDevice(Ptr<NetDevice> device, Ptr<Packet> packet, CustomHeader& ch)
{
    if (ch.l3Prot == 0xFB)
    {
        // std::cout << "CNCP packet received at switch " << GetId() << std::endl;
        // std::cout << "source ip: " << ch.cncp.sip << std::endl;
        // std::cout << "destination ip: " << ch.cncp.dip << std::endl;
        // std::cout << "source port: " << ch.cncp.sport << std::endl;
        // std::cout << "destination port: " << ch.cncp.dport << std::endl;
        // std::cout << "protocol: " << ch.cncp.protocol << std::endl;
        // std::cout << "flow info: " << ch.cncp.flowInfo << std::endl;
        FlowKey key;
        key.sip = ch.cncp.sip;
        key.dip = ch.cncp.dip;
        key.sport = ch.cncp.sport;
        key.dport = ch.cncp.dport;
        key.protocol = ch.cncp.protocol;
        key.priority_group = ch.cncp.priority_group;
        CNCPUpdateFromReport(key, ch.cncp.flowInfo);
    }
    else
    {
        SendToDev(device, packet, ch);
    }
    return true;
}

void
SwitchNode::SwitchNotifyDequeue(uint32_t ifIndex, uint32_t qIndex, Ptr<Packet> p)
{
    FlowIdTag t;
    p->PeekPacketTag(t);
    CustomHeader ch;
    p->RemoveHeader(ch);
    if (qIndex != 0)
    {
        uint32_t inDev = t.GetFlowId();
        m_mmu->RemoveFromIngressAdmission(inDev, qIndex, p->GetSize());
        m_mmu->RemoveFromEgressAdmission(ifIndex, qIndex, p->GetSize());
        m_bytes[inDev][ifIndex][qIndex] -= p->GetSize();
        if (m_ecnEnabled)
        {
            bool egressCongested = m_mmu->ShouldSendCN(ifIndex, qIndex);
            if (egressCongested)
            {
                PppHeader ppp;
                Ipv4Header h;
                p->RemoveHeader(ppp);
                p->RemoveHeader(h);
                h.SetEcn((Ipv4Header::EcnType)0x03);
                p->AddHeader(h);
                p->AddHeader(ppp);
            }
        }
        CheckAndSendResume(inDev, qIndex);
    }
    if (1)
    {
        uint8_t* buf = p->GetBuffer();
        if (buf[PppHeader::GetStaticSize() + 9] == 0x11)
        { // udp packet
            IntHeader* ih = (IntHeader*)&buf[PppHeader::GetStaticSize() + 20 + 8 +
                                             6]; // ppp, ip, udp, SeqTs, INT
            Ptr<QbbNetDevice> dev = DynamicCast<QbbNetDevice>(m_devices[ifIndex]);
            if (m_ccMode == 3)
            { // HPCC
                ih->PushHop(Simulator::Now().GetTimeStep(),
                            m_txBytes[ifIndex],
                            dev->GetQueue()->GetNBytesTotal(),
                            dev->GetDataRate().GetBitRate());
            }
            else if (m_ccMode == 10)
            { // HPCC-PINT
                uint64_t t = Simulator::Now().GetTimeStep();
                uint64_t dt = t - m_lastPktTs[ifIndex];
                if (dt > m_maxRtt)
                {
                    dt = m_maxRtt;
                }
                uint64_t B = dev->GetDataRate().GetBitRate() / 8; // Bps
                uint64_t qlen = dev->GetQueue()->GetNBytesTotal();
                double newU;

                /**************************
                 * approximate calc
                 *************************/
                int b = 20, m = 16, l = 20; // see log2apprx's paremeters
                int sft = logres_shift(b, l);
                double fct = 1 << sft;               // (multiplication factor corresponding to sft)
                double log_T = log2(m_maxRtt) * fct; // log2(T)*fct
                double log_B = log2(B) * fct;        // log2(B)*fct
                double log_1e9 = log2(1e9) * fct;    // log2(1e9)*fct
                double qterm = 0;
                double byteTerm = 0;
                double uTerm = 0;
                if ((qlen >> 8) > 0)
                {
                    int log_dt = log2apprx(dt, b, m, l);          // ~log2(dt)*fct
                    int log_qlen = log2apprx(qlen >> 8, b, m, l); // ~log2(qlen / 256)*fct
                    qterm = pow(2, (log_dt + log_qlen + log_1e9 - log_B - 2 * log_T) / fct) * 256;
                    // 2^((log2(dt)*fct+log2(qlen/256)*fct+log2(1e9)*fct-log2(B)*fct-2*log2(T)*fct)/fct)*256
                    // ~= dt*qlen*1e9/(B*T^2)
                }
                if (m_lastPktSize[ifIndex] > 0)
                {
                    int byte = m_lastPktSize[ifIndex];
                    int log_byte = log2apprx(byte, b, m, l);
                    byteTerm = pow(2, (log_byte + log_1e9 - log_B - log_T) / fct);
                    // 2^((log2(byte)*fct+log2(1e9)*fct-log2(B)*fct-log2(T)*fct)/fct) ~= byte*1e9 /
                    // (B*T)
                }
                if (m_maxRtt > dt && m_u[ifIndex] > 0)
                {
                    int log_T_dt = log2apprx(m_maxRtt - dt, b, m, l); // ~log2(T-dt)*fct
                    int log_u =
                        log2apprx(int(round(m_u[ifIndex] * 8192)), b, m, l); // ~log2(u*512)*fct
                    uTerm = pow(2, (log_T_dt + log_u - log_T) / fct) / 8192;
                    // 2^((log2(T-dt)*fct+log2(u*512)*fct-log2(T)*fct)/fct)/512 = (T-dt)*u/T
                }
                newU = qterm + byteTerm + uTerm;

#if 0
				/**************************
				 * accurate calc
				 *************************/
				double weight_ewma = double(dt) / m_maxRtt;
				double u;
				if (m_lastPktSize[ifIndex] == 0)
					u = 0;
				else{
					double txRate = m_lastPktSize[ifIndex] / double(dt); // B/ns
					u = (qlen / m_maxRtt + txRate) * 1e9 / B;
				}
				newU = m_u[ifIndex] * (1 - weight_ewma) + u * weight_ewma;
				printf(" %lf\n", newU);
#endif

                /************************
                 * update PINT header
                 ***********************/
                uint16_t power = Pint::encode_u(newU);
                if (power > ih->GetPower())
                {
                    ih->SetPower(power);
                }

                m_u[ifIndex] = newU;
            }
        }
    }
    m_txBytes[ifIndex] += p->GetSize();
    m_lastPktSize[ifIndex] = p->GetSize();
    m_lastPktTs[ifIndex] = Simulator::Now().GetTimeStep();
}

int
SwitchNode::logres_shift(int b, int l)
{
    static int data[] = {0, 0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
                         5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5};
    return l - data[b];
}

int
SwitchNode::log2apprx(int x, int b, int m, int l)
{
    int x0 = x;
    int msb = int(log2(x)) + 1;
    if (msb > m)
    {
        x = (x >> (msb - m) << (msb - m));
#if 0
		x += + (1 << (msb - m - 1));
#else
        int mask = (1 << (msb - m)) - 1;
        if ((x0 & mask) > (rand() & mask))
        {
            x += 1 << (msb - m);
        }
#endif
    }
    return int(log2(x) * (1 << logres_shift(b, l)));
}

bool
SwitchNode::CNCPAdmitIngress(Ptr<Packet> packet, CustomHeader& ch)
{
    // If CNCP is not enabled, or it is ack or nack packet, or it is background flow (pg=2), always admit
    if (m_ccMode != 11 || ch.l3Prot == 0xFC || ch.l3Prot == 0xFD || ch.udp.pg == 2)
    {
        return true;
    }
    // obtain flow key from custom header
    uint32_t sip = ch.sip;
    uint32_t dip = ch.dip;
    uint16_t sport = 0, dport = 0;
    uint8_t protocol = ch.l3Prot;
    if (ch.l3Prot == 0x6)
    { // TCP
        sport = ch.tcp.sport;
        dport = ch.tcp.dport;
    }
    else if (ch.l3Prot == 0x11)
    { // UDP
        sport = ch.udp.sport;
        dport = ch.udp.dport;
    }

    FlowKey key;
    key.sip = sip;
    key.dip = dip;
    key.sport = sport;
    key.dport = dport;
    key.protocol = protocol;
    key.priority_group = ch.udp.pg;
    // check if flow is already in the table
    auto it = m_flowControlRateTable.find(key);
    // admission control to simulate flow control
    if (it != m_flowControlRateTable.end())
    { // flow is already in the table
        uint64_t flowRate = it->second;
        uint64_t packetSize = packet->GetSize();
        // get timestamp of the last packet
        uint64_t lastPktTs = m_flowLastIngressPktTsTable[key];
        uint64_t currentTs = Simulator::Now().GetTimeStep();
        uint64_t dt = currentTs - lastPktTs;
        uint64_t bytesWindow =
            m_flowIngressWindowTable[key] + (flowRate * dt) / (8ULL * 1000000000ULL);
        // NS_LOG_DEBUG("Node " << this->GetId() << " check packet:" << "port: " << key.dport
        //                      << ", seq: " << ch.udp.seq << ", bytesWindow: " << bytesWindow
        //                      << ", packetSize: " << packetSize << " with dt: " << dt);
        if (bytesWindow < packetSize)
        {
            return false;
        }
        else
        { // packets comming, update flow control information
            // update ingress bytes window
            m_flowIngressWindowTable[key] = bytesWindow - packetSize;
            // update last packet timestamp
            m_flowLastIngressPktTsTable[key] = currentTs;
            return true;
        }
    }
    else
    { // flow is not in the table, means it is a new flow, always admit
        // NS_LOG_DEBUG("Node " << this->GetId() << "admit new flow:" << "protocol: "
        //                      << (int)key.protocol << ", sip: " << key.sip << ", dip: " << key.dip
        //                      << ", sport: " << key.sport << ", dport: " << key.dport
        //                      << ", pg: " << (int)key.priority_group);
        m_flowLastIngressPktTsTable[key] = Simulator::Now().GetTimeStep();
        return true;
    }
}

void
SwitchNode::CNCPNotifyIngress(Ptr<Packet> packet,
                              CustomHeader& ch,
                              uint32_t output_dev_idx,
                              Ptr<NetDevice> input_device)
{
    // obtain flow key from custom header
    uint32_t sip = ch.sip;
    uint32_t dip = ch.dip;
    uint16_t sport = 0, dport = 0;
    uint8_t protocol = ch.l3Prot;
    if (ch.l3Prot == 0x6)
    { // TCP
        sport = ch.tcp.sport;
        dport = ch.tcp.dport;
    }
    else if (ch.l3Prot == 0x11)
    { // UDP
        sport = ch.udp.sport;
        dport = ch.udp.dport;
    }
    else if (ch.l3Prot == 0xFC || ch.l3Prot == 0xFD)
    { // ACK/NACK
        sport = ch.ack.sport;
        dport = ch.ack.dport;
    }
    FlowKey key;
    key.sip = sip;
    key.dip = dip;
    key.sport = sport;
    key.dport = dport;
    key.protocol = protocol;
    key.priority_group = ch.udp.pg;
    uint64_t flowRate = 0;
    // check if flow is already in the table
    auto it = m_flowControlRateTable.find(key);
    // admission control to simulate flow control
    if (it != m_flowControlRateTable.end())
    { // flow is already in the table, do nothing here. Because we don't know if it should be admitted or not here.
        flowRate = it->second;
    }
    else
    { // flow is not in the table, means new flow arrives, reallocate flow rate
        uint64_t currentTs = Simulator::Now().GetTimeStep();
        m_flowIngressWindowTable[key] = packet->GetSize(); // saved for the first packet
        m_flowEgressDevIdxTable[key] = output_dev_idx;
        // get device rate
        Ptr<QbbNetDevice> device = DynamicCast<QbbNetDevice>(m_devices[output_dev_idx]);
        uint64_t totalRate = device->GetDataRate().GetBitRate();
        // count flows on the same egress device
        size_t activeFlowCount = 0;
        for (const auto& kv : m_flowEgressDevIdxTable) {
            if (kv.second == output_dev_idx) {
                ++activeFlowCount;
            }
        }
        // reallocate flow rate
        flowRate = totalRate / (activeFlowCount);
        for (auto& it : m_flowControlRateTable)
        {
            // Use find to avoid inserting a default value if the key does not exist in m_flowEgressDevIdxTable
            auto egressIt = m_flowEgressDevIdxTable.find(it.first);
            if (egressIt != m_flowEgressDevIdxTable.end() && egressIt->second == output_dev_idx) {
                it.second = flowRate;
            }
        }
        m_flowControlRateTable[key] = flowRate;
        m_flowBytesOnNodeTable[key] = m_default_flow_capacity_on_node;
        m_flowPrevHopDevTable[key] = input_device;
        m_flowQvTable[key] = m_default_flow_capacity_on_node;
        m_flowLastIngressPktTsTable[key] = currentTs;
        // Schedule a check event, if the flow is expired, remove it from the table
        Simulator::Schedule(NanoSeconds(m_cncp_flow_expired_interval),
                            &SwitchNode::CNCPCheckFlowExpired,
                            this,
                            key);
        // Schedule a update event, if the flow is not updated for a while, update the flow rate
        Simulator::Schedule(NanoSeconds(m_cncp_update_interval),
                            &SwitchNode::CNCPUpdate,
                            this,
                            key);
        // Schedule a report event, if the flow is not reported for a while, report the flow rate
        Simulator::Schedule(NanoSeconds(m_cncp_report_interval),
                            &SwitchNode::ReportCNCPStatus,
                            this,
                            key);
    }

    // m_flowBytesOnNodeTable[key] += packet->GetSize();

    // update Q_u for CNCP flow control
    uint64_t lastPktTs = m_flowLastArrivalPktTsTable[key];
    uint64_t currentTs = Simulator::Now().GetTimeStep();
    uint64_t dt = currentTs - lastPktTs;
    int32_t deltaQ = flowRate * dt / (8ULL * 1000000000ULL) - packet->GetSize();
    m_flowBytesOnNodeTable[key] += deltaQ;
    // NS_LOG_DEBUG("Node " << GetId() << " CNCPNotifyIngress: key=(" << key.dport << "), update Qu as " << m_flowBytesOnNodeTable[key] << " with flow rate " << flowRate << " and dt " << dt);
    if (m_flowBytesOnNodeTable[key] < 0)
    {
        m_flowBytesOnNodeTable[key] = 0;
    }
    if (m_flowBytesOnNodeTable[key] > m_default_flow_capacity_on_node)
    {
        m_flowBytesOnNodeTable[key] = m_default_flow_capacity_on_node;
    }
    m_flowLastArrivalPktTsTable[key] = currentTs;
    return;
}

void
SwitchNode::CNCPCheckFlowExpired(FlowKey key)
{
    // check if the flow is expired
    uint64_t currentTs = Simulator::Now().GetTimeStep();
    uint64_t lastPktTs = m_flowLastArrivalPktTsTable[key];
    uint64_t dt = currentTs - lastPktTs;
    if (dt > m_cncp_flow_expired_interval)
    {
        m_flowIngressWindowTable.erase(key);
        m_flowLastIngressPktTsTable.erase(key);
        m_flowLastArrivalPktTsTable.erase(key);
        m_flowBytesOnNodeTable.erase(key);
        // share this flow's rate with other flows
        uint64_t rate = m_flowControlRateTable.erase(key);
        size_t activeFlowCount = m_flowControlRateTable.size();
        if (activeFlowCount > 1)
        { // there are other flows on the node
            uint64_t sharedRate = rate / (activeFlowCount - 1);
            for (auto& it : m_flowControlRateTable)
            {
                it.second += sharedRate;
            }
        }
        m_flowControlRateTable.erase(key);
        m_flowPrevHopDevTable.erase(key);
        m_flowEgressDevIdxTable.erase(key);
        m_flowQvTable.erase(key);
    }else{
        Simulator::Schedule(NanoSeconds(m_cncp_flow_expired_interval),
                            &SwitchNode::CNCPCheckFlowExpired,
                            this,
                            key);
    }
}

void
SwitchNode::ReportCNCPStatus(FlowKey key)
{
    // find the flow in the control rate table, if not found, do not schedule a report event
    auto flow = m_flowControlRateTable.find(key);
    if (flow == m_flowControlRateTable.end())
    {
        return;
    }else{
        // Get the bytes for this flow from m_flowBytesOnNodeTable
        auto bytesIt = m_flowBytesOnNodeTable.find(key);
        if (bytesIt == m_flowBytesOnNodeTable.end())
        {
            return;
        }
        Ptr<NetDevice> device = m_flowPrevHopDevTable[key];
        if (device)
        {
            Ptr<QbbNetDevice> qbbDevice = DynamicCast<QbbNetDevice>(device);
            if (qbbDevice)
            {
                qbbDevice->SendCNCPReport(key, bytesIt->second);
            }
        }

        // Schedule a report event, if the flow is not reported for a while, report the flow rate
        Simulator::Schedule(NanoSeconds(m_cncp_report_interval),
                            &SwitchNode::ReportCNCPStatus,
                            this,
                            key);
    }
}

void
SwitchNode::CNCPUpdate(FlowKey key)
{
    // find the flow in the control rate table
    auto flow = m_flowControlRateTable.find(key);
    if (flow == m_flowControlRateTable.end())
    {
        return;
    }else{
    
        FlowKey key = flow->first;
        uint64_t f_e = flow->second;

        // Get q_v from flowQvTable
        int64_t q_v = m_flowQvTable[key];
        if (q_v < 0)
        {
            q_v = 0;
        }

        // Get q_u from m_flowBytesOnNodeTable
        int64_t q_u = m_flowBytesOnNodeTable[key];
        if (q_u < 0)
        {
            q_u = 0;
        }

        // p_e(t) and q_u^i(t) are set to 0 for now (can be extended later)
        // look up entries
        uint64_t p_e = 0;
        auto entry = m_rtTable.find(key.dip);
        if (entry != m_rtTable.end())
        {
            // Create a dummy packet for GetOutDev
            Ptr<Packet> dummyPacket = Create<Packet>();

            // Create and setup CustomHeader
            CustomHeader ch;
            ch.sip = key.sip;
            ch.dip = key.dip;
            ch.l3Prot = key.protocol;

            // Set protocol specific fields
            if (key.protocol == 0x6)
            { // TCP
                ch.tcp.sport = key.sport;
                ch.tcp.dport = key.dport;
            }
            else if (key.protocol == 0x11)
            { // UDP
                ch.udp.sport = key.sport;
                ch.udp.dport = key.dport;
                ch.udp.pg = key.priority_group;
            }
            else if (key.protocol == 0xFC || key.protocol == 0xFD)
            { // ACK/NACK
                ch.ack.sport = key.sport;
                ch.ack.dport = key.dport;
            }
            // Get the output device index
            int dev_idx = GetOutDev(dummyPacket, ch);
            if (dev_idx >= 0)
            {
                Ptr<QbbNetDevice> device = DynamicCast<QbbNetDevice>(m_devices[dev_idx]);
                p_e = device->GetQueueLength(key.priority_group);
            }
        }

        // Get the flow rate for the next iteration
        uint64_t f_e_new = CNCPGetNextIteration(f_e, q_v, p_e, q_u);

        // check if the flow rate is larger than egress device's rate
        int dev_idx = m_flowEgressDevIdxTable[key];
        Ptr<QbbNetDevice> device = DynamicCast<QbbNetDevice>(m_devices[dev_idx]);
        uint64_t egress_rate = device->GetDataRate().GetBitRate();
        // print egress rate
        if (f_e_new > egress_rate)
        {
            f_e_new = egress_rate;
        }

        // Update the flow rate in the table
        flow->second = f_e_new;

        // Schedule a update event, if the flow is not updated for a while, update the flow rate
        Simulator::Schedule(NanoSeconds(m_cncp_update_interval),
                            &SwitchNode::CNCPUpdate,
                            this,
                            key);

        // Print flow rate before and after update
        // double trans_gamma = 8 * m_gamma / m_cncp_report_interval;
        // NS_LOG_DEBUG(std::setw(4) << GetId() << " " 
        //             << std::setw(6) << key.dport << " "
        //             << std::setw(10) << std::left << "oldRate=" << std::setw(12) << f_e << ", "
        //             << std::setw(10) << std::left << "newRate=" << std::setw(12) << f_e_new << ", "
        //             << std::setw(6) << std::left << "q_v=" << std::setw(12) << q_v * trans_gamma << ", "
        //             << std::setw(6) << std::left << "p_e=" << std::setw(12) << p_e * trans_gamma << ", "
        //             << std::setw(6) << std::left << "q_u=" << std::setw(12) << q_u * trans_gamma << ", "
        //             << std::setw(10) << std::left << "U_prime=" << std::setw(12) << m_lambda / (f_e > 0 ? f_e : 1) << " "
        //             << Simulator::Now().GetTimeStep());
        // NS_LOG_DEBUG(GetId() << " " << key.dport << " " << f_e << " " << f_e_new << " " << Simulator::Now().GetTimeStep());
    }
}

uint64_t
SwitchNode::CNCPGetNextIteration(uint64_t f_e, uint64_t q_v, uint64_t p_e, uint64_t q_u)
{
    // Compute the derivative of the utility function U'(f_e^i(t))
    // Here we use U'(x) = 1/x
    double U_prime = m_lambda / (f_e > 0 ? f_e : 1); // avoid division by zero

    double trans_gamma = 8 * m_gamma / m_cncp_report_interval;
    double result = f_e + m_gamma * (q_v * trans_gamma + U_prime - 1.1 * p_e * trans_gamma - q_u * trans_gamma);
    // NS_LOG_DEBUG("U_prime=" << U_prime << ", f_e=" << f_e << ", q_v=" << q_v << ", p_e=" << p_e
    // << ", q_u=" << q_u << ", result=" << result);
    return result > 0 ? static_cast<uint64_t>(result) : 0;
}

void
SwitchNode::CNCPUpdateFromReport(FlowKey key, uint64_t flowInfo)
{
    m_flowQvTable[key] = flowInfo;
}

} /* namespace ns3 */
