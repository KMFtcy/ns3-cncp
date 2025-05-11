/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/header.h"
#include "ns3/simulator.h"
#include "rdma-seq-ts-header.h"

NS_LOG_COMPONENT_DEFINE ("RDMASeqTsHeader");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (RDMASeqTsHeader);

RDMASeqTsHeader::RDMASeqTsHeader ()
  : m_seq (0)
{
	if (IntHeader::mode == 1)
		ih.ts = Simulator::Now().GetTimeStep();
}

void
RDMASeqTsHeader::SetSeq (uint32_t seq)
{
  m_seq = seq;
}
uint32_t
RDMASeqTsHeader::GetSeq (void) const
{
  return m_seq;
}

void
RDMASeqTsHeader::SetPG (uint16_t pg)
{
	m_pg = pg;
}
uint16_t
RDMASeqTsHeader::GetPG (void) const
{
	return m_pg;
}

Time
RDMASeqTsHeader::GetTs (void) const
{
	NS_ASSERT_MSG(IntHeader::mode == 1, "SeqTsHeader cannot GetTs when IntHeader::mode != 1");
	return TimeStep (ih.ts);
}

TypeId
RDMASeqTsHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RDMASeqTsHeader")
    .SetParent<Header> ()
    .AddConstructor<RDMASeqTsHeader> ()
  ;
  return tid;
}
TypeId
RDMASeqTsHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
void
RDMASeqTsHeader::Print (std::ostream &os) const
{
  //os << "(seq=" << m_seq << " time=" << TimeStep (m_ts).GetSeconds () << ")";
	//os << m_seq << " " << TimeStep (m_ts).GetSeconds () << " " << m_pg;
	os << m_seq << " " << m_pg;
}
uint32_t
RDMASeqTsHeader::GetSerializedSize (void) const
{
	return GetHeaderSize();
}
uint32_t RDMASeqTsHeader::GetHeaderSize(void){
	return 6 + IntHeader::GetStaticSize();
}

void
RDMASeqTsHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteHtonU32 (m_seq);
  i.WriteHtonU16 (m_pg);

  // write IntHeader
  ih.Serialize(i);
}
uint32_t
RDMASeqTsHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_seq = i.ReadNtohU32 ();
  m_pg =  i.ReadNtohU16 ();

  // read IntHeader
  ih.Deserialize(i);
  return GetSerializedSize ();
}

} // namespace ns3