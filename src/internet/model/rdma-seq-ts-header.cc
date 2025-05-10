/*
 * Copyright (c) 2009 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "rdma-seq-ts-header.h"

#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RDMASeqTsHeader");

NS_OBJECT_ENSURE_REGISTERED(RDMASeqTsHeader);

RDMASeqTsHeader::RDMASeqTsHeader()
    : m_seq(0),
      m_ts(Simulator::Now().GetTimeStep())
{
    NS_LOG_FUNCTION(this);
}

void
RDMASeqTsHeader::SetSeq(uint32_t seq)
{
    NS_LOG_FUNCTION(this << seq);
    m_seq = seq;
}

uint32_t
RDMASeqTsHeader::GetSeq() const
{
    NS_LOG_FUNCTION(this);
    return m_seq;
}

void
RDMASeqTsHeader::SetPG(uint16_t pg)
{
    m_pg = pg;
}

uint16_t
RDMASeqTsHeader::GetPG(void) const
{
	return m_pg;
}

Time
RDMASeqTsHeader::GetTs() const
{
    NS_LOG_FUNCTION(this);
    return TimeStep(m_ts);
}

TypeId
RDMASeqTsHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RDMASeqTsHeader")
                            .SetParent<Header>()
                            .SetGroupName("Internet")
                            .AddConstructor<RDMASeqTsHeader>();
    return tid;
}

TypeId
RDMASeqTsHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
RDMASeqTsHeader::Print(std::ostream& os) const
{
    NS_LOG_FUNCTION(this << &os);
    os << "(seq=" << m_seq << " time=" << TimeStep(m_ts).As(Time::S) << ")";
}

uint32_t
RDMASeqTsHeader::GetSerializedSize() const
{
    NS_LOG_FUNCTION(this);
    return 4 + 8;
}

void
RDMASeqTsHeader::Serialize(Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;
    i.WriteHtonU32(m_seq);
    i.WriteHtonU64(m_ts);
}

uint32_t
RDMASeqTsHeader::Deserialize(Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;
    m_seq = i.ReadNtohU32();
    m_ts = i.ReadNtohU64();
    return GetSerializedSize();
}

} // namespace ns3
