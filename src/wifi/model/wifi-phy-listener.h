/*
 * Copyright (c) 2005,2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef WIFI_PHY_LISTENER_H
#define WIFI_PHY_LISTENER_H

#include "wifi-phy-common.h"

#include <vector>

namespace ns3
{

/**
 * @brief receive notifications about PHY events.
 */
class WifiPhyListener
{
  public:
    /****************************************************************
     *       This destructor is needed.
     ****************************************************************/

    virtual ~WifiPhyListener()
    {
    }

    /**
     * @param duration the expected duration of the packet reception.
     *
     * We have received the first bit of a packet. We decided
     * that we could synchronize on this packet. It does not mean
     * we will be able to successfully receive completely the
     * whole packet. It means that we will report a BUSY status until
     * one of the following happens:
     *   - NotifyRxEndOk
     *   - NotifyRxEndError
     *   - NotifyTxStart
     */
    virtual void NotifyRxStart(Time duration) = 0;
    /**
     * We have received the last bit of a packet for which
     * NotifyRxStart was invoked first and, the packet has
     * been successfully received.
     */
    virtual void NotifyRxEndOk() = 0;
    /**
     * We have received the last bit of a packet for which
     * NotifyRxStart was invoked first and, the packet has
     * _not_ been successfully received.
     */
    virtual void NotifyRxEndError() = 0;
    /**
     * @param duration the expected transmission duration.
     * @param txPower the nominal TX power
     *
     * We are about to send the first bit of the packet.
     * We do not send any event to notify the end of
     * transmission. Listeners should assume that the
     * channel implicitly reverts to the idle state
     * unless they have received a CCA busy report.
     */
    virtual void NotifyTxStart(Time duration, dBm_u txPower) = 0;
    /**
     * @param duration the expected busy duration.
     * @param channelType the channel type for which the CCA busy state is reported.
     * @param per20MhzDurations vector that indicates for how long each 20 MHz subchannel
     *        (corresponding to the index of the element in the vector) is busy and where a zero
     * duration indicates that the subchannel is idle. The vector is non-empty if  the PHY supports
     * 802.11ax or later and if the operational channel width is larger than 20 MHz.
     *
     * This method does not really report a real state
     * change as opposed to the other methods in this class.
     * It merely reports that, unless the medium is reported
     * busy through NotifyTxStart or NotifyRxStart/End,
     * it will be busy as defined by the currently selected
     * CCA mode.
     *
     * Typical client code which wants to have a clear picture
     * of the CCA state will need to keep track of the time at
     * which the last NotifyCcaBusyStart method is called and
     * what duration it reported.
     */
    virtual void NotifyCcaBusyStart(Time duration,
                                    WifiChannelListType channelType,
                                    const std::vector<Time>& per20MhzDurations) = 0;
    /**
     * @param duration the expected channel switching duration.
     *
     * We do not send any event to notify the end of
     * channel switching. Listeners should assume that the
     * channel implicitly reverts to the idle or busy states.
     */
    virtual void NotifySwitchingStart(Time duration) = 0;
    /**
     * Notify listeners that we went to sleep
     */
    virtual void NotifySleep() = 0;
    /**
     * Notify listeners that we went to switch off
     */
    virtual void NotifyOff() = 0;
    /**
     * Notify listeners that we woke up
     */
    virtual void NotifyWakeup() = 0;
    /**
     * Notify listeners that we went to switch on
     */
    virtual void NotifyOn() = 0;
};

} // namespace ns3

#endif /* WIFI_PHY_LISTENER_H */
