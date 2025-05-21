#ifndef CNCP_FLOWKEY_H
#define CNCP_FLOWKEY_H

#include <cstdint>
#include <tuple>
#include <unordered_map>

namespace ns3
{
// Define FlowKey struct
struct FlowKey {
    uint32_t sip;
    uint32_t dip;
    uint16_t sport;
    uint16_t dport;
    uint8_t protocol;

    // Comparison operator, required by unordered_map
    bool operator==(const FlowKey& other) const {
        return std::tie(sip, dip, sport, dport, protocol) ==
               std::tie(other.sip, other.dip, other.sport, other.dport, other.protocol);
    }
};

// Custom hash function
struct FlowKeyHash {
    std::size_t operator()(const FlowKey& key) const {
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

} // namespace ns3
#endif // CNCP_FLOWKEY_H
