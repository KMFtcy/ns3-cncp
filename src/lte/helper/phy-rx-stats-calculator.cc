/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Jaume Nin <jnin@cttc.es>
 * modified by: Marco Miozzo <mmiozzo@cttc.es>
 *        Convert MacStatsCalculator in PhyRxStatsCalculator
 */

#include "phy-rx-stats-calculator.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/string.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("PhyRxStatsCalculator");

NS_OBJECT_ENSURE_REGISTERED(PhyRxStatsCalculator);

PhyRxStatsCalculator::PhyRxStatsCalculator()
    : m_dlRxFirstWrite(true),
      m_ulRxFirstWrite(true)
{
    NS_LOG_FUNCTION(this);
}

PhyRxStatsCalculator::~PhyRxStatsCalculator()
{
    NS_LOG_FUNCTION(this);
    if (m_dlRxOutFile.is_open())
    {
        m_dlRxOutFile.close();
    }

    if (m_ulRxOutFile.is_open())
    {
        m_ulRxOutFile.close();
    }
}

TypeId
PhyRxStatsCalculator::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::PhyRxStatsCalculator")
            .SetParent<LteStatsCalculator>()
            .SetGroupName("Lte")
            .AddConstructor<PhyRxStatsCalculator>()
            .AddAttribute("DlRxOutputFilename",
                          "Name of the file where the downlink results will be saved.",
                          StringValue("DlRxPhyStats.txt"),
                          MakeStringAccessor(&PhyRxStatsCalculator::SetDlRxOutputFilename),
                          MakeStringChecker())
            .AddAttribute("UlRxOutputFilename",
                          "Name of the file where the uplink results will be saved.",
                          StringValue("UlRxPhyStats.txt"),
                          MakeStringAccessor(&PhyRxStatsCalculator::SetUlRxOutputFilename),
                          MakeStringChecker());
    return tid;
}

void
PhyRxStatsCalculator::SetUlRxOutputFilename(std::string outputFilename)
{
    LteStatsCalculator::SetUlOutputFilename(outputFilename);
}

std::string
PhyRxStatsCalculator::GetUlRxOutputFilename()
{
    return LteStatsCalculator::GetUlOutputFilename();
}

void
PhyRxStatsCalculator::SetDlRxOutputFilename(std::string outputFilename)
{
    LteStatsCalculator::SetDlOutputFilename(outputFilename);
}

std::string
PhyRxStatsCalculator::GetDlRxOutputFilename()
{
    return LteStatsCalculator::GetDlOutputFilename();
}

void
PhyRxStatsCalculator::DlPhyReception(PhyReceptionStatParameters params)
{
    NS_LOG_FUNCTION(this << params.m_cellId << params.m_imsi << params.m_timestamp << params.m_rnti
                         << params.m_layer << params.m_mcs << params.m_size << params.m_rv
                         << params.m_ndi << params.m_correctness);
    NS_LOG_INFO("Write DL Rx Phy Stats in " << GetDlRxOutputFilename());

    if (m_dlRxFirstWrite)
    {
        m_dlRxOutFile.open(GetDlRxOutputFilename());
        if (!m_dlRxOutFile.is_open())
        {
            NS_LOG_ERROR("Can't open file " << GetDlRxOutputFilename());
            return;
        }
        m_dlRxFirstWrite = false;
        m_dlRxOutFile
            << "% time\tcellId\tIMSI\tRNTI\ttxMode\tlayer\tmcs\tsize\trv\tndi\tcorrect\tccId";
        m_dlRxOutFile << "\n";
    }

    m_dlRxOutFile << params.m_timestamp << "\t";
    m_dlRxOutFile << (uint32_t)params.m_cellId << "\t";
    m_dlRxOutFile << params.m_imsi << "\t";
    m_dlRxOutFile << params.m_rnti << "\t";
    m_dlRxOutFile << (uint32_t)params.m_txMode << "\t";
    m_dlRxOutFile << (uint32_t)params.m_layer << "\t";
    m_dlRxOutFile << (uint32_t)params.m_mcs << "\t";
    m_dlRxOutFile << params.m_size << "\t";
    m_dlRxOutFile << (uint32_t)params.m_rv << "\t";
    m_dlRxOutFile << (uint32_t)params.m_ndi << "\t";
    m_dlRxOutFile << (uint32_t)params.m_correctness << "\t";
    m_dlRxOutFile << (uint32_t)params.m_ccId << std::endl;
}

void
PhyRxStatsCalculator::UlPhyReception(PhyReceptionStatParameters params)
{
    NS_LOG_FUNCTION(this << params.m_cellId << params.m_imsi << params.m_timestamp << params.m_rnti
                         << params.m_layer << params.m_mcs << params.m_size << params.m_rv
                         << params.m_ndi << params.m_correctness);
    NS_LOG_INFO("Write UL Rx Phy Stats in " << GetUlRxOutputFilename());

    if (m_ulRxFirstWrite)
    {
        m_ulRxOutFile.open(GetUlRxOutputFilename());
        if (!m_ulRxOutFile.is_open())
        {
            NS_LOG_ERROR("Can't open file " << GetUlRxOutputFilename());
            return;
        }
        m_ulRxFirstWrite = false;
        m_ulRxOutFile << "% time\tcellId\tIMSI\tRNTI\tlayer\tmcs\tsize\trv\tndi\tcorrect\tccId";
        m_ulRxOutFile << "\n";
    }

    m_ulRxOutFile << params.m_timestamp << "\t";
    m_ulRxOutFile << (uint32_t)params.m_cellId << "\t";
    m_ulRxOutFile << params.m_imsi << "\t";
    m_ulRxOutFile << params.m_rnti << "\t";
    m_ulRxOutFile << (uint32_t)params.m_layer << "\t";
    m_ulRxOutFile << (uint32_t)params.m_mcs << "\t";
    m_ulRxOutFile << params.m_size << "\t";
    m_ulRxOutFile << (uint32_t)params.m_rv << "\t";
    m_ulRxOutFile << (uint32_t)params.m_ndi << "\t";
    m_ulRxOutFile << (uint32_t)params.m_correctness << "\t";
    m_ulRxOutFile << (uint32_t)params.m_ccId << std::endl;
}

void
PhyRxStatsCalculator::DlPhyReceptionCallback(Ptr<PhyRxStatsCalculator> phyRxStats,
                                             std::string path,
                                             PhyReceptionStatParameters params)
{
    NS_LOG_FUNCTION(phyRxStats << path);
    uint64_t imsi = 0;
    std::ostringstream pathAndRnti;
    pathAndRnti << path << "/" << params.m_rnti;
    std::string pathUePhy = path.substr(0, path.find("/ComponentCarrierMapUe"));
    if (phyRxStats->ExistsImsiPath(pathAndRnti.str()))
    {
        imsi = phyRxStats->GetImsiPath(pathAndRnti.str());
    }
    else
    {
        imsi = FindImsiFromLteNetDevice(pathUePhy);
        phyRxStats->SetImsiPath(pathAndRnti.str(), imsi);
    }

    params.m_imsi = imsi;
    phyRxStats->DlPhyReception(params);
}

void
PhyRxStatsCalculator::UlPhyReceptionCallback(Ptr<PhyRxStatsCalculator> phyRxStats,
                                             std::string path,
                                             PhyReceptionStatParameters params)
{
    NS_LOG_FUNCTION(phyRxStats << path);
    uint64_t imsi = 0;
    std::ostringstream pathAndRnti;
    std::string pathEnb = path.substr(0, path.find("/ComponentCarrierMap"));
    pathAndRnti << pathEnb << "/LteEnbRrc/UeMap/" << params.m_rnti;
    if (phyRxStats->ExistsImsiPath(pathAndRnti.str()))
    {
        imsi = phyRxStats->GetImsiPath(pathAndRnti.str());
    }
    else
    {
        imsi = FindImsiFromEnbRlcPath(pathAndRnti.str());
        phyRxStats->SetImsiPath(pathAndRnti.str(), imsi);
    }

    params.m_imsi = imsi;
    phyRxStats->UlPhyReception(params);
}

} // namespace ns3
