#include "cf-e2e-calculator.h"

#include <ns3/boolean.h>
#include <ns3/config.h>
#include <ns3/log.h>
#include <ns3/string.h>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("CfE2eCalculator");
NS_OBJECT_ENSURE_REGISTERED(CfE2eCalculator);

TypeId
CfE2eCalculator::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::CfE2eCalculator")
            .SetParent<Object>()
            .AddConstructor<CfE2eCalculator>()
            .AddAttribute("UeE2eOutFileName",
                          "Name of the file where the UE end-to-end data will be backed up.",
                          StringValue("UeE2eOutBackup"),
                          MakeStringAccessor(&CfE2eCalculator::m_ueE2eOutFileName),
                          MakeStringChecker())
            .AddAttribute("FirstWrite",
                          "Balabala",
                          BooleanValue(true),
                          MakeBooleanAccessor(&CfE2eCalculator::m_firstWrite),
                          MakeBooleanChecker())
            .AddAttribute("FileSuffix",
                          "The suffix of files",
                          StringValue(""),
                          MakeStringAccessor(&CfE2eCalculator::m_fileSuffix),
                          MakeStringChecker());
    return tid;
}

CfE2eCalculator::CfE2eCalculator()
// : m_firstWrite(true)
//   m_ueE2eOutFile("UeE2eOutBackup.csv")
{
    NS_LOG_FUNCTION(this);
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
                                  uint64_t upWlDelay,
                                  uint64_t upWdDelay,
                                  uint64_t queueDelay,
                                  uint64_t computingDelay,
                                  uint64_t dnWdDelay,
                                  uint64_t dnWlDelay)
{
    UpdateSpecificDelayStats(ueId, upWlDelay, m_uplinkWirelessDelay);
    UpdateSpecificDelayStats(ueId, upWdDelay, m_uplinkWiredDelay);
    UpdateSpecificDelayStats(ueId, queueDelay, m_queueDelay);
    UpdateSpecificDelayStats(ueId, computingDelay, m_computingDelay);
    UpdateSpecificDelayStats(ueId, dnWdDelay, m_downlinkWiredDelay);
    UpdateSpecificDelayStats(ueId, dnWlDelay, m_downlinkWirelessDelay);

    UpdateSpecificDelayStats(ueId,
                             upWlDelay + upWdDelay + queueDelay + computingDelay + dnWdDelay +
                                 dnWlDelay,
                             m_e2eDelay);

    if (CheckQoeStat(upWlDelay, upWdDelay, queueDelay, computingDelay, dnWdDelay, dnWlDelay))
    {
        RecordSuccessfulTask(ueId);
    }
}

std::vector<double>
CfE2eCalculator::GetUplinkWirelessDelayStats(uint64_t ueId)
{
    return GetSpecificDelayStats(ueId, m_uplinkWirelessDelay);
}

std::vector<double>
CfE2eCalculator::GetUplinkWiredDelayStats(uint64_t ueId)
{
    return GetSpecificDelayStats(ueId, m_uplinkWiredDelay);
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
CfE2eCalculator::GetDownlinkWiredDelayStats(uint64_t ueId)
{
    return GetSpecificDelayStats(ueId, m_downlinkWiredDelay);
}

std::vector<double>
CfE2eCalculator::GetDownlinkWirelessDelayStats(uint64_t ueId)
{
    return GetSpecificDelayStats(ueId, m_downlinkWirelessDelay);
}

std::vector<double>
CfE2eCalculator::GetE2eDelayStats(uint64_t ueId)
{
    return GetSpecificDelayStats(ueId, m_e2eDelay);
}

bool
CfE2eCalculator::CheckQoeStat(uint64_t upWlDelay,
                              uint64_t upWdDelay,
                              uint64_t queueDelay,
                              uint64_t computingDelay,
                              uint64_t dnWdDelay,
                              uint64_t dnWlDelay)
{
    // double commDelay = (double)(upWlDelay + upWdDelay + dnWdDelay + dnWlDelay) / 1e6;
    double commDelay = (double)(dnWdDelay + dnWlDelay) / 1e6;
    double compDelay = (double)(queueDelay + computingDelay) / 1e6;
    // double e2eDelay =
    //     (double)(upWlDelay + upWdDelay + queueDelay + computingDelay + dnWdDelay + dnWlDelay) / 1e6;
    double e2eDelay =
        (double)(queueDelay + computingDelay + dnWdDelay + dnWlDelay) / 1e6;

    return (commDelay < 16.6 && compDelay < 16.6 && e2eDelay < 16.6);
}

void
CfE2eCalculator::RecordSuccessfulTask(uint64_t ueId)
{
    auto it = m_qoeCount.find(ueId);
    if (it == m_qoeCount.end())
    {
        m_qoeCount[ueId] = 1;
    }
    else
    {
        m_qoeCount[ueId] += 1;
    }
}

uint64_t
CfE2eCalculator::GetNumberOfTimeData(uint64_t ueId)
{
    auto it = m_e2eDelay.find(ueId);
    if (it != m_e2eDelay.end())
    {
        return m_e2eDelay[ueId]->getCount();
    }
    else
    {
        return 0;
    }
}

uint64_t
CfE2eCalculator::GetNumberOfSuccTask(uint64_t ueId)
{
    auto it = m_qoeCount.find(ueId);
    if(it != m_qoeCount.end())
    {
        return m_qoeCount[ueId];
    }
    else
    {
        return 0;
    }
}

void
CfE2eCalculator::ResetResultForUe(uint64_t ueId)
{
    NS_LOG_FUNCTION(this);

    auto uplinkWlEntry = m_uplinkWirelessDelay.find(ueId);
    if (uplinkWlEntry != m_uplinkWirelessDelay.end())
    {
        m_uplinkWirelessDelay.erase(uplinkWlEntry);
    }

    auto uplinkWdEntry = m_uplinkWiredDelay.find(ueId);
    if (uplinkWdEntry != m_uplinkWiredDelay.end())
    {
        m_uplinkWiredDelay.erase(uplinkWdEntry);
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

    auto downlinkWlEntry = m_downlinkWirelessDelay.find(ueId);
    if (downlinkWlEntry != m_downlinkWirelessDelay.end())
    {
        m_downlinkWirelessDelay.erase(downlinkWlEntry);
    }

    auto downlinkWdEntry = m_downlinkWiredDelay.find(ueId);
    if (downlinkWdEntry != m_downlinkWiredDelay.end())
    {
        m_downlinkWiredDelay.erase(downlinkWdEntry);
    }

    auto e2eEntry = m_e2eDelay.find(ueId);
    if (e2eEntry != m_e2eDelay.end())
    {
        m_e2eDelay.erase(e2eEntry);
    }

    auto qoeEntry = m_qoeCount.find(ueId);
    if (qoeEntry != m_qoeCount.end())
    {
        m_qoeCount.erase(qoeEntry);
    }
}

void
CfE2eCalculator::SetUeE2eOutFileName(std::string fileName)
{
    m_ueE2eOutFileName = fileName;
}

std::string
CfE2eCalculator::GetUeE2eOutFileName()
{
    return m_ueE2eOutFileName;
}

void
CfE2eCalculator::BackupUeE2eResults(uint64_t ueId, uint64_t assoCellId, uint64_t compCellId)
{
    NS_LOG_FUNCTION(this << ueId << assoCellId << compCellId);

    std::ofstream ueE2eOutFile;

    NS_LOG_DEBUG("m_firstWrite " << m_firstWrite);

    std::string fileName = m_ueE2eOutFileName + m_fileSuffix + ".csv";
    if (m_firstWrite == true)
    {
        ueE2eOutFile.open(fileName);
        NS_LOG_DEBUG("Create " << m_ueE2eOutFileName);
        if (!ueE2eOutFile.is_open())
        {
            NS_LOG_ERROR("Can't open file " << GetUeE2eOutFileName().c_str());
            return;
        }

        m_firstWrite = false;

        ueE2eOutFile << "Time,"
                     << "ueId,"
                     << "assoCell,"
                     << "compCell,";
        ueE2eOutFile << "taskCount,";
        ueE2eOutFile << "succCount,";
        ueE2eOutFile << "upWlMea,upWlStd,upWlMin,upWlMax,"
                        "upWdMea,upWdStd,upWdMin,upWdMax,"
                        "queMea,queStd,queMin,queMax,"
                        "compMea, compStd, compMin, compMax,"
                        "dnWdMea, dnWdStd, dnWdMin, dnWdMax,"
                        "dnWlMea,dnWlStd, dnWlMin, dnWlMax,"
                        "e2eMea,e2eStd,e2eMin,e2eMax";

        ueE2eOutFile << std::endl;
    }

    else
    {
        ueE2eOutFile.open(fileName, std::ios_base::app);
        if (!ueE2eOutFile.is_open())
        {
            NS_LOG_ERROR("Can't open file " << fileName);
            return;
        }
        ueE2eOutFile << Simulator::Now().GetSeconds() << "," << ueId << "," << assoCellId << ","
                     << compCellId << ",";

        ueE2eOutFile << this->GetNumberOfTimeData(ueId) << ",";
        ueE2eOutFile << this->GetNumberOfSuccTask(ueId) << ",";

        std::vector<double> upWlDelay = this->GetUplinkWirelessDelayStats(ueId);
        std::vector<double> upWdDelay = this->GetUplinkWiredDelayStats(ueId);
        std::vector<double> queueDelay = this->GetQueueDelayStats(ueId);
        std::vector<double> compDelay = this->GetComputingDelayStats(ueId);
        std::vector<double> dnWdDelay = this->GetDownlinkWiredDelayStats(ueId);
        std::vector<double> dnWlDelay = this->GetDownlinkWirelessDelayStats(ueId);
        std::vector<double> e2eDelay = this->GetE2eDelayStats(ueId);

        std::vector<double> delayVector[] =
            {upWlDelay, upWdDelay, queueDelay, compDelay, dnWdDelay, dnWlDelay, e2eDelay};

        for (auto delay : delayVector)
        {
            for (auto delayData : delay)
            {
                ueE2eOutFile << delayData / 1e6 << ",";
            }
        }

        ueE2eOutFile << std::endl;

        NS_LOG_DEBUG("Backup e2e data of UE " << ueId);
    }
}
} // namespace ns3
