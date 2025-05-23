/*
 * Copyright (c) 2011 Bucknell University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: L. Felipe Perrone (perrone@bucknell.edu)
 *          Tiago G. Rodrigues (tgr002@bucknell.edu)
 *
 * Modified by: Mitch Watrous (watrous@u.washington.edu)
 * Adapted to Ipv6 by: Tommaso Pecorella (tommaso.pecorella@unifi.it)
 */

#ifndef IPV6_PACKET_PROBE_H
#define IPV6_PACKET_PROBE_H

#include "ipv6.h"

#include "ns3/boolean.h"
#include "ns3/callback.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/probe.h"
#include "ns3/simulator.h"
#include "ns3/traced-value.h"

namespace ns3
{

/**
 * @ingroup ipv6
 *
 * This class is designed to probe an underlying ns3 TraceSource
 * exporting a packet, an IPv6 object, and an interface.  This probe
 * exports a trace source "Output" with arguments of type Ptr<const Packet>,
 * Ptr<Ipv6>, and uint32_t.  The Output trace source emits a value
 * when either the trace source emits a new value, or when SetValue ()
 * is called.
 */
class Ipv6PacketProbe : public Probe
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    Ipv6PacketProbe();
    ~Ipv6PacketProbe() override;

    /**
     * @brief Set a probe value
     *
     * @param packet set the traced packet equal to this
     * @param ipv6 set the IPv6 object for the traced packet equal to this
     * @param interface set the IPv6 interface for the traced packet equal to this
     */
    void SetValue(Ptr<const Packet> packet, Ptr<Ipv6> ipv6, uint32_t interface);

    /**
     * @brief Set a probe value by its name in the Config system
     *
     * @param path config path to access the probe
     * @param packet set the traced packet equal to this
     * @param ipv6 set the IPv6 object for the traced packet equal to this
     * @param interface set the IPv6 interface for the traced packet equal to this
     */
    static void SetValueByPath(std::string path,
                               Ptr<const Packet> packet,
                               Ptr<Ipv6> ipv6,
                               uint32_t interface);

    /**
     * @brief connect to a trace source attribute provided by a given object
     *
     * @param traceSource the name of the attribute TraceSource to connect to
     * @param obj ns3::Object to connect to
     * @return true if the trace source was successfully connected
     */
    bool ConnectByObject(std::string traceSource, Ptr<Object> obj) override;

    /**
     * @brief connect to a trace source provided by a config path
     *
     * @param path Config path to bind to
     *
     * Note, if an invalid path is provided, the probe will not be connected
     * to anything.
     */
    void ConnectByPath(std::string path) override;

  private:
    /**
     * @brief Method to connect to an underlying ns3::TraceSource with
     * arguments of type Ptr<const Packet>, Ptr<Ipv6>, and uint32_t
     *
     * @param packet the traced packet
     * @param ipv6 the IPv6 object for the traced packet
     * @param interface the IPv6 interface for the traced packet
     */
    void TraceSink(Ptr<const Packet> packet, Ptr<Ipv6> ipv6, uint32_t interface);

    /// Traced Callback: the packet, the Ipv6 object and the interface.
    ns3::TracedCallback<Ptr<const Packet>, Ptr<Ipv6>, uint32_t> m_output;
    /// Traced Callback: the previous packet's size and the actual packet's size.
    ns3::TracedCallback<uint32_t, uint32_t> m_outputBytes;

    /// The traced packet.
    Ptr<const Packet> m_packet;

    /// The IPv6 object for the traced packet.
    Ptr<Ipv6> m_ipv6;

    /// The IPv6 interface for the traced packet.
    uint32_t m_interface;

    /// The size of the traced packet.
    uint32_t m_packetSizeOld;
};

} // namespace ns3

#endif // IPV6_PACKET_PROBE_H
