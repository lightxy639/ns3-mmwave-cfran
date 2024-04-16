#ifndef CF_E2E_CALCULATOR_H
#define CF_E2E_CALCULATOR_H

#include <ns3/basic-data-calculators.h>
#include <ns3/object.h>

#include <map>
#include <vector>

namespace ns3
{

typedef std::map<uint64_t, Ptr<MinMaxAvgTotalCalculator<uint64_t>>> DelayStatsMap;

class CfE2eCalculator : public Object
{
  public:
    CfE2eCalculator();

    virtual ~CfE2eCalculator();

    static TypeId GetTypeId();
    void DoDispose();

    void UpdateDelayStats(uint64_t ueId,
                          uint64_t upWlDelay,
                          uint64_t upWdDelay,
                          uint64_t queueDelay,
                          uint64_t computingDelay,
                          uint64_t dnWdDelay,
                          uint64_t dnWlDelay);

    void UpdateSpecificDelayStats(uint64_t ueId, uint64_t delay, DelayStatsMap& delayStatsMap);

    std::vector<double> GetSpecificDelayStats(uint64_t ueId, DelayStatsMap& delayStatsMap);

    std::vector<double> GetUplinkWirelessDelayStats(uint64_t ueId);
    std::vector<double> GetUplinkWiredDelayStats(uint64_t ueId);
    std::vector<double> GetQueueDelayStats(uint64_t ueId);
    std::vector<double> GetComputingDelayStats(uint64_t ueId);
    std::vector<double> GetDownlinkWirelessDelayStats(uint64_t ueId);
    std::vector<double> GetDownlinkWiredDelayStats(uint64_t ueId);
    std::vector<double> GetE2eDelayStats(uint64_t ueId);

    void ResetResultForUe(uint64_t ueId);

    void SetUeE2eOutFileName(std::string UeE2eOutFileName);
    std::string GetUeE2eOutFileName();
    
    void BackupUeE2eResults(uint64_t ueId, uint64_t assoCellId, uint64_t compCellId);

  private:
    // void ShowResults();
    // void WriteUeE2eResults(std::ofstream& outFile);

    DelayStatsMap m_uplinkWirelessDelay;
    DelayStatsMap m_uplinkWiredDelay;
    DelayStatsMap m_queueDelay;
    DelayStatsMap m_computingDelay;
    DelayStatsMap m_downlinkWiredDelay;
    DelayStatsMap m_downlinkWirelessDelay;
    DelayStatsMap m_e2eDelay;

    std::string m_ueE2eOutFileName;
    bool m_firstWrite;
};
} // namespace ns3

#endif