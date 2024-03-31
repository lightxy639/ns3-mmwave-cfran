#ifndef CF_E2E_CALCULATOR_H
#define CF_E2E_CALCULATOR_H

#include <ns3/object.h>
#include <ns3/basic-data-calculators.h>

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
                          uint64_t uplinkDelay,
                          uint64_t queueDelay,
                          uint64_t computingDelay,
                          uint64_t downlinkDelay);

    void UpdateSpecificDelayStats(uint64_t ueId, uint64_t delay, DelayStatsMap& delayStatsMap);

    std::vector<double> GetSpecificDelayStats(uint64_t ueId, DelayStatsMap& delayStatsMap);

    std::vector<double> GetUplinkDelayStats(uint64_t ueId);
    std::vector<double> GetQueueDelayStats(uint64_t ueId);
    std::vector<double> GetComputingDelayStats(uint64_t ueId);
    std::vector<double> GetDownlinkDelayStats(uint64_t ueId);
    std::vector<double> GetE2eDelayStats(uint64_t ueId);

    void ResetResultForUe(uint64_t ueId);

  private:
    DelayStatsMap m_uplinkDelay;
    DelayStatsMap m_queueDelay;
    DelayStatsMap m_computingDelay;
    DelayStatsMap m_downlinkDelay;
    DelayStatsMap m_e2eDelay;

};
} // namespace ns3

#endif