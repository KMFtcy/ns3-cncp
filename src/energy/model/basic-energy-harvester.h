/*
 * Copyright (c) 2014 Wireless Communications and Networking Group (WCNG),
 * University of Rochester, Rochester, NY, USA.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Cristiano Tapparello <cristiano.tapparello@rochester.edu>
 */

#ifndef BASIC_ENERGY_HARVESTER
#define BASIC_ENERGY_HARVESTER

#include "device-energy-model.h"
#include "energy-harvester.h"

#include "ns3/event-id.h"
#include "ns3/nstime.h"
#include "ns3/random-variable-stream.h"
#include "ns3/traced-value.h"

#include <iostream>

namespace ns3
{
namespace energy
{

/**
 * @ingroup energy
 * BasicEnergyHarvester increases remaining energy stored in an associated
 * Energy Source. The BasicEnergyHarvester implements a simple model in which
 * the amount of power provided by the harvester varies over time according
 * to a customizable generic random variable and time update intervals.
 *
 * Unit of power is chosen as Watt since energy models typically calculate
 * energy as (time in seconds * power in Watt).
 *
 */
class BasicEnergyHarvester : public EnergyHarvester
{
  public:
    /**
     * @brief Get the type ID.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    BasicEnergyHarvester();

    /**
     * @param updateInterval Energy harvesting update interval.
     *
     * BasicEnergyHarvester constructor function that sets the interval
     * between each update of the value of the power harvested by this
     * energy harvester.
     */
    BasicEnergyHarvester(Time updateInterval);

    ~BasicEnergyHarvester() override;

    /**
     * @param updateInterval Energy harvesting update interval.
     *
     * This function sets the interval between each update of the value of the
     * power harvested by this energy harvester.
     */
    void SetHarvestedPowerUpdateInterval(Time updateInterval);

    /**
     * @returns The interval between each update of the harvested power.
     *
     * This function returns the interval between each update of the value of the
     * power harvested by this energy harvester.
     */
    Time GetHarvestedPowerUpdateInterval() const;

    /**
     * @param stream Random variable stream number.
     * @returns The number of stream indices assigned by this model.
     *
     * This function sets the stream number to be used by the random variable that
     * determines the amount of power that can be harvested by this energy harvester.
     */
    int64_t AssignStreams(int64_t stream);

  private:
    /// Defined in ns3::Object
    void DoInitialize() override;

    /// Defined in ns3::Object
    void DoDispose() override;

    /**
     * Calculates harvested Power.
     */
    void CalculateHarvestedPower();

    /**
     * @returns m_harvestedPower The power currently provided by the Basic Energy Harvester.
     * Implements DoGetPower defined in EnergyHarvester.
     */
    double DoGetPower() const override;

    /**
     * This function is called every m_energyHarvestingUpdateInterval in order to
     * update the amount of power that will be provided by the harvester in the
     * next interval.
     */
    void UpdateHarvestedPower();

  private:
    Ptr<RandomVariableStream> m_harvestablePower; //!< Random variable for the harvestable power

    TracedValue<double> m_harvestedPower;        //!< current harvested power, in Watt
    TracedValue<double> m_totalEnergyHarvestedJ; //!< total harvested energy, in Joule

    EventId m_energyHarvestingUpdateEvent; //!< energy harvesting event
    Time m_lastHarvestingUpdateTime;       //!< last harvesting time
    Time m_harvestedPowerUpdateInterval;   //!< harvestable energy update interval
};

} // namespace energy
} // namespace ns3

#endif /* defined(BASIC_ENERGY_HARVESTER) */
