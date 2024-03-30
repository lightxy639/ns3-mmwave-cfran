#ifndef CF_E2E_CALCULATOR_H
#define CF_E2E_CALCULATOR_H

#include <ns3/object.h>
#include <ns3/basic-data-calculators.h>

#include <map>
#include <vector>

namespace ns3
{
struct UeTaskIdPair_t
{
    uint64_t m_ueId;
    uint64_t m_taskId;

  public:
    UeTaskIdPair_t(){};
    UeTaskIdPair_t(const uint64_t a, const uint64_t b)
        : m_ueId(a),
          m_taskId(b){};

    friend bool operator==(const UeTaskIdPair_t& a, const UeTaskIdPair_t& b)
    {
        return ((a.m_ueId == b.m_ueId) && (a.m_taskId == b.m_taskId));
    }
};

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

    void UpdateSpecificDelayStats(uint64_t imsi, uint64_t delay, DelayStatsMap& delayStatsMap);

    std::vector<double> GetSpecificDelayStats(uint64_t imsi, DelayStatsMap& delayStatsMap);

    std::vector<double> GetUplinkDelayStats(uint64_t imsi);
    std::vector<double> GetQueueDelayStats(uint64_t imsi);
    std::vector<double> GetComputingDelayStats(uint64_t imsi);
    std::vector<double> GetDownlinkDelayStats(uint64_t imsi);
    std::vector<double> GetE2eDelayStats(uint64_t imsi);

  private:
    DelayStatsMap m_uplinkDelay;
    DelayStatsMap m_queueDelay;
    DelayStatsMap m_computingDelay;
    DelayStatsMap m_downlinkDelay;
    DelayStatsMap m_e2eDelay;
};
} // namespace ns3

#endif