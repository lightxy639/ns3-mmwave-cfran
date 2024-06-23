#ifndef CF_E2E_BUFFER_H
#define CF_E2E_BUFFER_H

#include <ns3/object.h>

#include <map>

namespace ns3
{

// struct UeTaskIdPair_t
// {
//     uint64_t m_ueId;
//     uint64_t m_taskId;

//   public:
//     UeTaskIdPair_t(){};
//     UeTaskIdPair_t(const uint64_t a, const uint64_t b)
//         : m_ueId(a),
//           m_taskId(b){};

//     friend bool operator==(const UeTaskIdPair_t& a, const UeTaskIdPair_t& b)
//     {
//         return ((a.m_ueId == b.m_ueId) && (a.m_taskId == b.m_taskId));
//     }

//     friend bool operator<(const UeTaskIdPair_t& a, const UeTaskIdPair_t& b)
//     {
//         return ((a.m_ueId < b.m_ueId) || ((a.m_ueId == b.m_ueId) && (a.m_taskId < b.m_taskId)));
//     }
// };

typedef std::map<UeTaskIdPair_t, uint64_t> DetailedDelayStatsBuffer;

class CfE2eBuffer : public Object
{
  public:
    CfE2eBuffer();

    virtual ~CfE2eBuffer();

    static TypeId GetTypeId();
    void DoDispose();

    // void UpdateUplinkDelayBuffer(uint64_t ueId, uint64_t taskId, uint64_t delay);
    // void UpdateQueueDelayBuffer(uint64_t ueId, uint64_t taskId, uint64_t delay);
    // void UpdateComputingDelayBuffer(uint64_t ueId, uint64_t taskId, uint64_t delay);
    // void UpdateDownlinkDelayBuffer(uint64_t ueId, uint64_t taskId, uint64_t delay);
    // // void UpdatE2eDelayBuffer(uint64_t ueId, uint64_t taskId, uint64_t delay);

    // void GetUplinkDelayBuffer(uint64_t ueId, uint64_t taskId, bool erase = false);
    // void GetQueueDelayBuffer(uint64_t ueId, uint64_t taskId, bool erase = false);
    // void GetComputingDelayBuffer(uint64_t ueId, uint64_t taskId, bool erase = false);
    // void GetDownlinkDelayBuffer(uint64_t ueId, uint64_t taskId, bool erase = false);
    // void GetE2eDelayBuffer(uint64_t ueId, uint64_t taskId, bool erase = false);

    uint64_t GetUplinkWirelessDelay(uint64_t ueId, uint64_t taskId, bool erase = false);
    void UpdateUplinkWirelessDelay(uint64_t ueId, uint64_t taskId, uint64_t delay);
    uint64_t GetUplinkWiredDelay(uint64_t ueId, uint64_t taskId, bool erase = false);
    void UpdateUplinkWiredDelay(uint64_t ueId, uint64_t taskId, uint64_t delay);
    uint64_t GetQueueDelay(uint64_t ueId, uint64_t taskId, bool erase = false);
    void UpdateQueueDelay(uint64_t ueId, uint64_t taskId, uint64_t delay);
    uint64_t GetComputingDelay(uint64_t ueId, uint64_t taskId, bool erase = false);
    void UpdateComputingDelay(uint64_t ueId, uint64_t taskId, uint64_t delay);
    uint64_t GetDownlinkWiredDelay(uint64_t ueId, uint64_t taskId, bool erase = false);
    void UpdateDownlinkWiredDelay(uint64_t ueId, uint64_t taskId, uint64_t delay);
    uint64_t GetDownlinkWirelessDelay(uint64_t ueId, uint64_t taskId, bool erase = false);
    void UpdateDownlinkWirelessDelay(uint64_t ueId, uint64_t taskId, uint64_t delay);

    uint64_t GetSpecificDelayBuffer(uint64_t ueId,
                                    uint64_t taskId,
                                    DetailedDelayStatsBuffer& delayStatsMap,
                                    bool erase = false);
    void UpdateSpecificDelayBuffer(uint64_t ueId,
                                   uint64_t taskId,
                                   uint64_t delay,
                                   DetailedDelayStatsBuffer& delayStatsMap);



    static void UeSendRequestCallback(Ptr<CfE2eBuffer> stats,
                                      uint64_t ueId,
                                      uint64_t taskId,
                                      uint64_t delay);
    static void GnbRecvUeRequestCallback(Ptr<CfE2eBuffer> stats,
                                         uint64_t ueId,
                                         uint64_t taskId,
                                         uint64_t delay);

    static void GnbForwardUeRequestCallback(Ptr<CfE2eBuffer> stats,
                                            uint64_t ueId,
                                            uint64_t taskId,
                                            uint64_t delay);
    static void GnbRecvForwardedUeRequestCallback(Ptr<CfE2eBuffer> stats,
                                                  uint64_t ueId,
                                                  uint64_t taskId,
                                                  uint64_t delay);

    static void CfAppAddTaskCallback(Ptr<CfE2eBuffer> stats,
                                     uint64_t ueId,
                                     uint64_t taskId,
                                     uint64_t delay);
    static void CfUnitStartCompCallback(Ptr<CfE2eBuffer> stats,
                                        uint64_t ueId,
                                        uint64_t taskId,
                                        uint64_t delay);
    static void CfAppGetResultCallback(Ptr<CfE2eBuffer> stats,
                                       uint64_t ueId,
                                       uint64_t taskId,
                                       uint64_t delay);

    static void GnbForwardResultCallback(Ptr<CfE2eBuffer> stats,
                                         uint64_t ueId,
                                         uint64_t taskId,
                                         uint64_t delay);
    static void GnbGetForwardedResultCallback(Ptr<CfE2eBuffer> stats,
                                              uint64_t ueId,
                                              uint64_t taskId,
                                              uint64_t delay);

    static void GnbSendResultToUeCallback(Ptr<CfE2eBuffer> stats,
                                          uint64_t ueId,
                                          uint64_t taskId,
                                          uint64_t delay);
    static void UeRecvResultCallback(Ptr<CfE2eBuffer> stats,
                                     uint64_t ueId,
                                     uint64_t taskId,
                                     uint64_t delay);

  private:
    // DetailedDelayStatsBuffer m_uplinkDelay;
    // DetailedDelayStatsBuffer m_queueDelay;
    // DetailedDelayStatsBuffer m_computingDelay;
    // DetailedDelayStatsBuffer m_downlinkDelay;
    // DetailedDelayStatsBuffer m_e2eDelay;
    DetailedDelayStatsBuffer m_uplinkWirelessDelay;
    DetailedDelayStatsBuffer m_uplinkWiredDelay;
    DetailedDelayStatsBuffer m_queueDelay;
    DetailedDelayStatsBuffer m_computingDelay;
    DetailedDelayStatsBuffer m_downlinkWiredDelay;
    DetailedDelayStatsBuffer m_downlinkWirelessDelay;
};
} // namespace ns3

#endif