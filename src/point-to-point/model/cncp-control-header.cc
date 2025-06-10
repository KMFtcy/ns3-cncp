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
  : m_sip (0),
    m_dip (0),
    m_sport (0),
    m_dport (0),
    m_protocol (0),
    m_flow_info (0)
{
}

CncpControlHeader::CncpControlHeader (uint32_t sip, uint32_t dip, uint16_t sport, uint16_t dport, uint8_t protocol, uint8_t priority_group, uint64_t flowInfo)
  : m_sip (sip),
    m_dip (dip),
    m_sport (sport),
    m_dport (dport),
    m_protocol (protocol),
    m_priority_group (priority_group),
    m_flow_info (flowInfo)
{
}

CncpControlHeader::~CncpControlHeader ()
{
}

void
CncpControlHeader::SetSourceIp (uint32_t sip)
{
  m_sip = sip;
}

void
CncpControlHeader::SetDestIp (uint32_t dip)
{
  m_dip = dip;
}

void
CncpControlHeader::SetSourcePort (uint16_t sport)
{
  m_sport = sport;
}

void
CncpControlHeader::SetDestPort (uint16_t dport)
{
  m_dport = dport;
}

void
CncpControlHeader::SetProtocol (uint8_t protocol)
{
  m_protocol = protocol;
}

void
CncpControlHeader::SetFlowInfo (uint64_t flowInfo)
{
  m_flow_info = flowInfo;
}

uint32_t
CncpControlHeader::GetSourceIp (void) const
{
  return m_sip;
}

uint32_t
CncpControlHeader::GetDestIp (void) const
{
  return m_dip;
}

uint16_t
CncpControlHeader::GetSourcePort (void) const
{
  return m_sport;
}

uint16_t
CncpControlHeader::GetDestPort (void) const
{
  return m_dport;
}

uint8_t
CncpControlHeader::GetProtocol (void) const
{
  return m_protocol;
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
  os << "sip=" << m_sip 
     << ", dip=" << m_dip
     << ", sport=" << m_sport
     << ", dport=" << m_dport
     << ", protocol=" << (uint32_t)m_protocol
     << ", priority_group=" << (uint32_t)m_priority_group
     << ", flow_info=" << m_flow_info;
}

uint32_t
CncpControlHeader::GetSerializedSize (void) const
{
  return 22; // 4 bytes for sip + 4 bytes for dip + 2 bytes for sport + 2 bytes for dport + 1 byte for protocol + 1 byte for priority_group + 8 bytes for flow_info
}

void
CncpControlHeader::Serialize (Buffer::Iterator start) const
{
  start.WriteU32 (m_sip);
  start.WriteU32 (m_dip);
  start.WriteU16 (m_sport);
  start.WriteU16 (m_dport);
  start.WriteU8 (m_protocol);
  start.WriteU8 (m_priority_group);
  start.WriteU64 (m_flow_info);
}

uint32_t
CncpControlHeader::Deserialize (Buffer::Iterator start)
{
  m_sip = start.ReadU32 ();
  m_dip = start.ReadU32 ();
  m_sport = start.ReadU16 ();
  m_dport = start.ReadU16 ();
  m_protocol = start.ReadU8 ();
  m_priority_group = start.ReadU8 ();
  m_flow_info = start.ReadU64 ();
  return GetSerializedSize ();
}

} // namespace ns3
