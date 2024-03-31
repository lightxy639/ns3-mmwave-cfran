#include "cf-e2e-calculator.h"

#include <ns3/config.h>
#include <ns3/log.h>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("CfE2eCalculator");
NS_OBJECT_ENSURE_REGISTERED(CfE2eCalculator);

CfE2eCalculator::CfE2eCalculator()
{
    NS_LOG_FUNCTION(this);
}

TypeId
CfE2eCalculator::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::CfE2eCalculator").SetParent<Object>().AddConstructor<CfE2eCalculator>();
    return tid;
}

void
CfE2eCalculator::DoDispose()
{
    NS_LOG_FUNCTION(this);
}

CfE2eCalculator::~CfE2eCalculator()
{
    NS_LOG_FUNCTION(this);
}

void
CfE2eCalculator::UpdateSpecificDelayStats(uint64_t ueId,
                                          uint64_t delay,
                                          DelayStatsMap& delayStatsMap)
{
    DelayStatsMap::iterator it = delayStatsMap.find(ueId);

    if (it == delayStatsMap.end())
    {
        delayStatsMap[ueId] = CreateObject<MinMaxAvgTotalCalculator<uint64_t>>();
        delayStatsMap[ueId]->Update(delay);
    }
    else
    {
        it->second->Update(delay);
    }
}

std::vector<double>
CfE2eCalculator::GetSpecificDelayStats(uint64_t ueId, DelayStatsMap& delayStatsMap)
{
    NS_LOG_FUNCTION(this);
    std::vector<double> stats;
    DelayStatsMap::iterator it = delayStatsMap.find(ueId);

    if (it == delayStatsMap.end())
    {
        stats.push_back(0.0);
        stats.push_back(0.0);
        stats.push_back(0.0);
        stats.push_back(0.0);
        return stats;
    }
    stats.push_back(delayStatsMap[ueId]->getMean());
    stats.push_back(delayStatsMap[ueId]->getStddev());
    stats.push_back(delayStatsMap[ueId]->getMin());
    stats.push_back(delayStatsMap[ueId]->getMax());
    return stats;
}

void
CfE2eCalculator::UpdateDelayStats(uint64_t ueId,
                                  uint64_t uplinkDelay,
                                  uint64_t queueDelay,
                                  uint64_t computingDelay,
                                  uint64_t downlinkDelay)
{
    UpdateSpecificDelayStats(ueId, uplinkDelay, m_uplinkDelay);
    UpdateSpecificDelayStats(ueId, queueDelay, m_queueDelay);
    UpdateSpecificDelayStats(ueId, computingDelay, m_computingDelay);
    UpdateSpecificDelayStats(ueId, downlinkDelay, m_downlinkDelay);
    UpdateSpecificDelayStats(ueId,
                             uplinkDelay + queueDelay + computingDelay + downlinkDelay,
                             m_e2eDelay);
}

std::vector<double>
CfE2eCalculator::GetUplinkDelayStats(uint64_t ueId)
{
    return GetSpecificDelayStats(ueId, m_uplinkDelay);
}

std::vector<double>
CfE2eCalculator::GetQueueDelayStats(uint64_t ueId)
{
    return GetSpecificDelayStats(ueId, m_queueDelay);
}

std::vector<double>
CfE2eCalculator::GetComputingDelayStats(uint64_t ueId)
{
    return GetSpecificDelayStats(ueId, m_computingDelay);
}

std::vector<double>
CfE2eCalculator::GetDownlinkDelayStats(uint64_t ueId)
{
    return GetSpecificDelayStats(ueId, m_downlinkDelay);
}

std::vector<double>
CfE2eCalculator::GetE2eDelayStats(uint64_t ueId)
{
    return GetSpecificDelayStats(ueId, m_e2eDelay);
}

void
CfE2eCalculator::ResetResultForUe(uint64_t ueId)
{
    NS_LOG_FUNCTION(this);

    auto uplinkEntry = m_uplinkDelay.find(ueId);
    if (uplinkEntry != m_uplinkDelay.end())
    {
        m_uplinkDelay.erase(uplinkEntry);
    }

    auto queueEntry = m_queueDelay.find(ueId);
    if (queueEntry != m_queueDelay.end())
    {
        m_queueDelay.erase(queueEntry);
    }

    auto computingEntry = m_computingDelay.find(ueId);
    if (computingEntry != m_computingDelay.end())
    {
        m_computingDelay.erase(computingEntry);
    }

    auto downlinkEntry = m_downlinkDelay.find(ueId);
    if (downlinkEntry != m_downlinkDelay.end())
    {
        m_downlinkDelay.erase(downlinkEntry);
    }

    auto e2eEntry = m_e2eDelay.find(ueId);
    if (e2eEntry != m_e2eDelay.end())
    {
        m_e2eDelay.erase(e2eEntry);
    }
}

} // namespace ns3
