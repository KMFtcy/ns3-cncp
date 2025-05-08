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

#ifndef BROADCOM_EGRESS_H
#define BROADCOM_EGRESS_H

#include <vector>
#include "ns3/packet.h"
#include "ns3/queue.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/traced-callback.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"

namespace ns3 {

class TraceContainer;

/**
 * \ingroup network
 * \brief Broadcom-like multi-queue egress queue
 *
 * This class implements a hardware-style egress queue with multiple
 * queues (up to 128), and Round-Robin + priority-based dequeue logic,
 * inspired by Broadcom switch/NIC behavior.
 */
class BEgressQueue : public Queue<Packet>
{
public:
  static TypeId GetTypeId (void);

  static const unsigned fCnt = 128; //!< Maximum number of queues (for NIC)
  static const unsigned qCnt = 8;   //!< Maximum round-robin queues (for switch)

public:
  BEgressQueue ();
  virtual ~BEgressQueue ();

  /** Enqueue packet into a specific queue */
  bool Enqueue (Ptr<Packet> p, uint32_t qIndex);

  /** Dequeue one packet using priority + RR scheduling */
  Ptr<Packet> DequeueRR (bool paused[]);

  /** Get bytes in a specific queue */
  uint32_t GetNBytes (uint32_t qIndex) const;

  /** Get total bytes in all queues */
  uint32_t GetNBytesTotal () const;

  /** Get last dequeued queue index */
  uint32_t GetLastQueue ();

  /** Trace callback for enqueue to BEgressQueue (packet, queueIndex) */
  TracedCallback<Ptr<const Packet>, uint32_t> m_traceBeqEnqueue;

  /** Trace callback for dequeue from BEgressQueue (packet, queueIndex) */
  TracedCallback<Ptr<const Packet>, uint32_t> m_traceBeqDequeue;

private:
  /** Internal enqueue used by public Enqueue */
  bool DoEnqueue (Ptr<Packet> p, uint32_t qIndex);

  /** Internal Round-Robin dequeue */
  Ptr<Packet> DoDequeueRR (bool paused[]);

  /** Compatibility: enqueue without priority (fallback to queue 0) */
  bool Enqueue (Ptr<Packet> p) override;

  /** Compatibility: not implemented */
  Ptr<Packet> Dequeue (void) override;

  /** Compatibility: not implemented */
  Ptr<Packet> Remove (void) override;

  /** Compatibility: (fallback to queue 0)*/
  Ptr<const Packet> Peek (void) const override;

  NS_LOG_TEMPLATE_DECLARE; //!< redefinition of the log component
private:
  double m_maxBytes; //!< Total byte limit across all queues

  uint32_t m_bytesInQueue[fCnt]; //!< Per-queue byte count
  uint32_t m_bytesInQueueTotal;         //!< Total byte count

  uint32_t m_rrlast;  //!< Last round-robin queue index
  uint32_t m_qlast;   //!< Last dequeued queue index

  std::vector<Ptr<Queue<Packet>>> m_queues; //!< The actual queues
};

} // namespace ns3

#endif /* BROADCOM_EGRESS_H */
