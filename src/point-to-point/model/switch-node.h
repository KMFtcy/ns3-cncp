#ifndef SWITCH_NODE_H
#define SWITCH_NODE_H

#include "pint.h"
#include "qbb-net-device.h"
#include "switch-mmu.h"

#include <ns3/node.h>

#include <unordered_map>

namespace ns3
{

class Packet;

// Define FlowKey struct
struct FlowKey
{
    uint32_t sip;
    uint32_t dip;
    uint16_t sport;
    uint16_t dport;
    uint8_t protocol;

    // Comparison operator, required by unordered_map
    bool operator==(const FlowKey& other) const
    {
        return std::tie(sip, dip, sport, dport, protocol) ==
               std::tie(other.sip, other.dip, other.sport, other.dport, other.protocol);
    }
};

// Custom hash function
struct FlowKeyHash
{
    std::size_t operator()(const FlowKey& key) const
    {
        std::size_t h1 = std::hash<uint32_t>{}(key.sip);
        std::size_t h2 = std::hash<uint32_t>{}(key.dip);
        std::size_t h3 = std::hash<uint16_t>{}(key.sport);
        std::size_t h4 = std::hash<uint16_t>{}(key.dport);
        std::size_t h5 = std::hash<uint8_t>{}(key.protocol);
        // Combine hash values
        std::size_t result = h1;
        result ^= h2 + 0x9e3779b9 + (result << 6) + (result >> 2);
        result ^= h3 + 0x9e3779b9 + (result << 6) + (result >> 2);
        result ^= h4 + 0x9e3779b9 + (result << 6) + (result >> 2);
        result ^= h5 + 0x9e3779b9 + (result << 6) + (result >> 2);
        return result;
    }
};

class SwitchNode : public Node
{
    static const uint32_t pCnt = 257; // Number of ports used
    static const uint32_t qCnt = 8;   // Number of queues/priorities used
    uint32_t m_ecmpSeed;
    std::unordered_map<uint32_t, std::vector<int>>
        m_rtTable; // map from ip address (u32) to possible ECMP port (index of dev)

    // monitor of PFC
    uint32_t m_bytes[pCnt][pCnt][qCnt]; // m_bytes[inDev][outDev][qidx] is the bytes from inDev
                                        // enqueued for outDev at qidx

    uint64_t m_txBytes[pCnt]; // counter of tx bytes

    uint32_t m_lastPktSize[pCnt];
    uint64_t m_lastPktTs[pCnt]; // ns
    double m_u[pCnt];

    // Flow control table for CNCP, key is flow id and value is target flow rate
    // for iterative update
    // uint64_t m_cncp_report_interval = 1000; // 0.001ms
    // std::unordered_map<FlowKey, Ptr<NetDevice>, FlowKeyHash> m_flowPrevHopDevTable;
    // uint32_t m_gamma = 0.1;
    // uint32_t m_lambda = 0.1;
    // for flow rate control
    std::unordered_map<FlowKey, uint64_t, FlowKeyHash> m_flowControlRateTable;
    std::unordered_map<FlowKey, uint64_t, FlowKeyHash> m_flowIngressWindowTable;
    std::unordered_map<FlowKey, uint64_t, FlowKeyHash> m_flowLastPktTsTable;
    std::unordered_map<FlowKey, uint64_t, FlowKeyHash> m_flowBytesOnNodeTable;

  protected:
    bool m_ecnEnabled;
    uint32_t m_ccMode;
    uint64_t m_maxRtt;

    uint32_t m_ackHighPrio; // set high priority for ACK/NACK

  private:
    int GetOutDev(Ptr<const Packet>, CustomHeader& ch);
    void SendToDev(Ptr<Packet> p, CustomHeader& ch);
    static uint32_t EcmpHash(const uint8_t* key, size_t len, uint32_t seed);
    void CheckAndSendPfc(uint32_t inDev, uint32_t qIndex);
    void CheckAndSendResume(uint32_t inDev, uint32_t qIndex);

  public:
    Ptr<SwitchMmu> m_mmu;

    static TypeId GetTypeId(void);
    SwitchNode();
    void SetEcmpSeed(uint32_t seed);
    void AddTableEntry(Ipv4Address& dstAddr, uint32_t intf_idx);
    void ClearTable();
    bool SwitchReceiveFromDevice(Ptr<NetDevice> device, Ptr<Packet> packet, CustomHeader& ch);
    void SwitchNotifyDequeue(uint32_t ifIndex, uint32_t qIndex, Ptr<Packet> p);

    // for approximate calc in PINT
    int logres_shift(int b, int l);
    int log2apprx(int x, int b, int m, int l); // given x of at most b bits, use most significant m
                                               // bits of x, calc the result in l bits

    // CNCP Flow Control
    bool CNCPAdmitIngress(Ptr<Packet> packet, CustomHeader& ch);
};

} /* namespace ns3 */

#endif /* SWITCH_NODE_H */
