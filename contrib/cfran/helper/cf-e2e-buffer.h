#ifndef CF_E2E_BUFFER_H
#define CF_E2E_BUFFER_H

#include <ns3/object.h>

#include <map>

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

    friend bool operator<(const UeTaskIdPair_t& a, const UeTaskIdPair_t& b)
    {
        return ((a.m_ueId < b.m_ueId) || ((a.m_ueId == b.m_ueId) && (a.m_taskId < b.m_taskId)));
    }
};

typedef std::map<UeTaskIdPair_t, uint64_t> DetailedTimeStatsBuffer;

class CfE2eBuffer : public Object
{
  public:
    CfE2eBuffer();

    virtual ~CfE2eBuffer();

    static TypeId GetTypeId();
    void DoDispose();

    void UpdateUplinkTimeBuffer(uint64_t ueId, uint64_t taskId, uint64_t time);
    void UpdateQueueTimeBuffer(uint64_t ueId, uint64_t taskId, uint64_t time);
    void UpdateComputingTimeBuffer(uint64_t ueId, uint64_t taskId, uint64_t time);
    void UpdateDownlinkTimeBuffer(uint64_t ueId, uint64_t taskId, uint64_t time);
    // void UpdatE2eTimeBuffer(uint64_t ueId, uint64_t taskId, uint64_t time);

    void GetUplinkTimeBuffer(uint64_t ueId, uint64_t taskId, bool erase);
    void GetQueueTimeBuffer(uint64_t ueId, uint64_t taskId, bool erase);
    void GetComputingTimeBuffer(uint64_t ueId, uint64_t taskId, bool erase);
    void GetDownlinkTimeBuffer(uint64_t ueId, uint64_t taskId, bool erase);
    // void GetE2eTimeBuffer(uint64_t ueId, uint64_t taskId, bool erase);

    uint64_t GetSpecificTimeBuffer(uint64_t ueId,
                                   uint64_t taskId,
                                   DetailedTimeStatsBuffer& timeStatsMap,
                                   bool erase);
    void UpdateSpecificTimeBuffer(uint64_t ueId,
                                  uint64_t taskId,
                                  uint64_t time,
                                  DetailedTimeStatsBuffer& timeStatsMap);

    static void TxRequestCallback(Ptr<CfE2eBuffer> stats, uint64_t ueId, uint64_t taskId, uint64_t time);
    // static void RxResultCallback(Ptr<CfE2eBuffer> stats, uint64_t ueId, uint64_t taskId, uint64_t time);
    static void QueueCallback(Ptr<CfE2eBuffer> stats, uint64_t ueId, uint64_t taskId, uint64_t time);
    static void ComputingCallback(Ptr<CfE2eBuffer> stats, uint64_t ueId, uint64_t taskId, uint64_t time);
    static void TxResultCallback(Ptr<CfE2eBuffer> stats, uint64_t ueId, uint64_t taskId, uint64_t time);

  private:
    DetailedTimeStatsBuffer m_uplinkTime;
    DetailedTimeStatsBuffer m_queueTime;
    DetailedTimeStatsBuffer m_computingTime;
    DetailedTimeStatsBuffer m_downlinkTime;
    // DetailedTimeStatsBuffer m_e2eTime;
};
} // namespace ns3

#endif