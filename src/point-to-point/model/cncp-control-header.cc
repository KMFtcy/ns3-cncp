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

#include "cncp-control-header.h"
#include "ns3/assert.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (CncpControlHeader);

CncpControlHeader::CncpControlHeader ()
  : m_flow_id (0),
    m_flow_info (0)
{
}

CncpControlHeader::CncpControlHeader (uint32_t flowId, uint64_t flowInfo)
  : m_flow_id (flowId),
    m_flow_info (flowInfo)
{
}

CncpControlHeader::~CncpControlHeader ()
{
}

void
CncpControlHeader::SetFlowId (uint32_t flowId)
{
  m_flow_id = flowId;
}

void
CncpControlHeader::SetFlowInfo (uint64_t flowInfo)
{
  m_flow_info = flowInfo;
}

uint32_t
CncpControlHeader::GetFlowId (void) const
{
  return m_flow_id;
}

uint64_t
CncpControlHeader::GetFlowInfo (void) const
{
  return m_flow_info;
}

TypeId
CncpControlHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CncpControlHeader")
    .SetParent<Header> ()
    .AddConstructor<CncpControlHeader> ()
    ;
  return tid;
}

TypeId
CncpControlHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
CncpControlHeader::Print (std::ostream &os) const
{
  os << "flow_id=" << m_flow_id << ", flow_info=" << m_flow_info;
}

uint32_t
CncpControlHeader::GetSerializedSize (void) const
{
  return 12; // 4 bytes for flow_id + 8 bytes for flow_info
}

void
CncpControlHeader::Serialize (Buffer::Iterator start) const
{
  start.WriteU32 (m_flow_id);
  start.WriteU64 (m_flow_info);
}

uint32_t
CncpControlHeader::Deserialize (Buffer::Iterator start)
{
  m_flow_id = start.ReadU32 ();
  m_flow_info = start.ReadU64 ();
  return GetSerializedSize ();
}

} // namespace ns3
