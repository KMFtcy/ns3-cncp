/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 New York University
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Adrian S.-W. Tam <adrian.sw.tam@gmail.com>
 */

#ifndef CNCP_CONTROL_HEADER_H
#define CNCP_CONTROL_HEADER_H

#include "ns3/buffer.h"
#include "ns3/header.h"

#include <stdint.h>

namespace ns3
{

/**
 * \ingroup Cncp
 * \brief Header for the Congestion Notification Message
 *
 * This class has two fields: The five-tuple flow id and the quantized
 * flow information. This can be serialized to or deserialzed from a byte
 * buffer.
 */

class CncpControlHeader : public Header
{
  public:
    CncpControlHeader();
    CncpControlHeader(uint32_t sip, uint32_t dip, uint16_t sport, uint16_t dport, uint8_t protocol, uint8_t priority_group, uint64_t flowInfo);
    virtual ~CncpControlHeader();

    // Setters
    void SetFlowInfo(uint64_t flowInfo);
    void SetSourceIp(uint32_t sip);
    void SetDestIp(uint32_t dip);
    void SetSourcePort(uint16_t sport);
    void SetDestPort(uint16_t dport);
    void SetProtocol(uint8_t protocol);

    // Getters
    uint64_t GetFlowInfo(void) const;
    uint32_t GetSourceIp(void) const;
    uint32_t GetDestIp(void) const;
    uint16_t GetSourcePort(void) const;
    uint16_t GetDestPort(void) const;
    uint8_t GetProtocol(void) const;

    static TypeId GetTypeId(void);
    virtual TypeId GetInstanceTypeId(void) const;
    virtual void Print(std::ostream& os) const;
    virtual uint32_t GetSerializedSize(void) const;
    virtual void Serialize(Buffer::Iterator start) const;
    virtual uint32_t Deserialize(Buffer::Iterator start);

  private:
    uint32_t m_sip;
    uint32_t m_dip;
    uint16_t m_sport;
    uint16_t m_dport;
    uint8_t m_protocol;
    uint8_t m_priority_group;
    uint64_t m_flow_info;
};

}; // namespace ns3

#endif /* CNCP_CONTROL_HEADER */
