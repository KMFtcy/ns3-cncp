/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006 Georgia Tech Research Corporation, INRIA
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
 */
#include "broadcom-egress-queue.h"

#include "drop-tail-queue.h"

#include "ns3/double.h"
#include "ns3/enum.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/traced-value.h"
#include "ns3/trace-source-accessor.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("BEgressQueue");
NS_OBJECT_ENSURE_REGISTERED(BEgressQueue);

TypeId
BEgressQueue::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::BEgressQueue")
            .SetParent<Queue<Packet>>()
            .AddConstructor<BEgressQueue>()
            .AddAttribute("MaxBytes",
                          "The maximum number of bytes accepted by this BEgressQueue.",
                          DoubleValue(1000.0 * 1024 * 1024),
                          MakeDoubleAccessor(&BEgressQueue::m_maxBytes),
                          MakeDoubleChecker<double>())
            .AddTraceSource("BeqEnqueue",
                            "Enqueue a packet in the BEgressQueue (multi-queue)",
                            MakeTraceSourceAccessor(&BEgressQueue::m_traceBeqEnqueue),
                            "ns3::Queue::EnqueueTracedCallback")
            .AddTraceSource("BeqDequeue",
                            "Dequeue a packet in the BEgressQueue (multi-queue)",
                            MakeTraceSourceAccessor(&BEgressQueue::m_traceBeqDequeue),
                            "ns3::Queue::DequeueTracedCallback");
    return tid;
}

BEgressQueue::BEgressQueue()
    : Queue<Packet>(),
      NS_LOG_TEMPLATE_DEFINE("BEgressQueue")
{
    NS_LOG_FUNCTION_NOARGS();
    m_bytesInQueueTotal = 0;
    m_rrlast = 0;
    for (uint32_t i = 0; i < fCnt; i++)
    {
        m_bytesInQueue[i] = 0;
        m_queues.push_back(CreateObject<DropTailQueue<Packet>>());
    }
}

BEgressQueue::~BEgressQueue()
{
    NS_LOG_FUNCTION_NOARGS();
}

// -------------------------------------------------------------------------
// Public multi-queue Enqueue/Dequeue interface
// -------------------------------------------------------------------------

bool
BEgressQueue::Enqueue(Ptr<Packet> p, uint32_t qIndex)
{
    NS_LOG_FUNCTION(this << p);

    bool retval = DoEnqueue(p, qIndex);
    if (retval)
    {
        // m_traceEnqueue(p);
        // m_traceBeqEnqueue(p, qIndex);

        uint32_t size = p->GetSize();
        m_nBytes += size;
        m_nTotalReceivedBytes += size;
        m_nPackets++;
        m_nTotalReceivedPackets++;
    }
    return retval;
}

Ptr<Packet>
BEgressQueue::DequeueRR(bool paused[])
{
    NS_LOG_FUNCTION(this);
    Ptr<Packet> packet = DoDequeueRR(paused);
    if (packet != nullptr)
    {
        NS_ASSERT(m_nBytes >= packet->GetSize());
        NS_ASSERT(m_nPackets > 0);
        m_nBytes -= packet->GetSize();
        m_nPackets--;
        // m_traceDequeue(packet);
    }
    return packet;
}

// -------------------------------------------------------------------------
// Internal helpers
// -------------------------------------------------------------------------

bool
BEgressQueue::DoEnqueue(Ptr<Packet> p, uint32_t qIndex)
{
    NS_LOG_FUNCTION(this << p << qIndex);

    if (m_bytesInQueueTotal + p->GetSize() < m_maxBytes)
    {
        m_queues[qIndex]->Enqueue(p);
        m_bytesInQueueTotal += p->GetSize();
        m_bytesInQueue[qIndex] += p->GetSize();
        return true;
    }

    return false;
}

Ptr<Packet>
BEgressQueue::DoDequeueRR(bool paused[])
{
    NS_LOG_FUNCTION(this);

    if (m_bytesInQueueTotal == 0)
    {
        NS_LOG_LOGIC("Queue empty");
        return nullptr;
    }

    bool found = false;
    uint32_t qIndex = 0;

    if (m_queues[0]->GetNPackets() > 0)
    {
        found = true;
        qIndex = 0;
    }
    else
    {
        for (uint32_t i = 1; i <= qCnt; i++)
        {
            uint32_t idx = (i + m_rrlast) % qCnt;
            if (!paused[idx] && m_queues[idx]->GetNPackets() > 0)
            {
                found = true;
                qIndex = idx;
                break;
            }
        }
    }

    if (found)
    {
        Ptr<Packet> p = m_queues[qIndex]->Dequeue();
        m_traceBeqDequeue(p, qIndex);
        m_bytesInQueueTotal -= p->GetSize();
        m_bytesInQueue[qIndex] -= p->GetSize();

        if (qIndex != 0)
        {
            m_rrlast = qIndex;
        }

        m_qlast = qIndex;
        NS_LOG_LOGIC("Dequeued from queue " << qIndex);
        return p;
    }

    NS_LOG_LOGIC("Nothing can be sent");
    return nullptr;
}

// -------------------------------------------------------------------------
// Compatibility with Queue<Packet> interface
// -------------------------------------------------------------------------

bool
BEgressQueue::Enqueue(Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this << p);
    std::cout << "Warning: Call Broadcom queues without priority\n";
    return Enqueue(p, 0); // fallback to queue 0
}

Ptr<Packet>
BEgressQueue::Dequeue()
{
    NS_ASSERT_MSG(false, "BEgressQueue::Dequeue without priority not implemented");
    return nullptr;
}

Ptr<Packet>
BEgressQueue::Remove()
{
    NS_ASSERT_MSG(false, "BEgressQueue::Remove without priority not implemented");
    return nullptr;
}

Ptr<const Packet>
BEgressQueue::Peek() const
{
    NS_LOG_FUNCTION(this);
    std::cout << "Warning: Call Broadcom queues without priority\n";

    if (m_bytesInQueueTotal == 0)
    {
        NS_LOG_LOGIC("Queue empty");
        return nullptr;
    }

    return m_queues[0]->Peek();
}

// -------------------------------------------------------------------------
// Accessors
// -------------------------------------------------------------------------

uint32_t
BEgressQueue::GetNBytes(uint32_t qIndex) const
{
    return m_bytesInQueue[qIndex];
}

uint32_t
BEgressQueue::GetNBytesTotal() const
{
    return m_bytesInQueueTotal;
}

uint32_t
BEgressQueue::GetLastQueue()
{
    return m_qlast;
}

} // namespace ns3