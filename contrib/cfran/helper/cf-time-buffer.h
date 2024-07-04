#ifndef CF_TIME_BUFFER
#define CF_TIME_BUFFER

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

enum OffloadPosition
{
    None = 0,
    LocalGnb,
    OtherGnb,
    RemoteServer
};

enum TimeType
{
    SendRequest = 0,
    RecvResult,
    RecvRequest,
    SendResult,
    RecvRequestToBeForwarded,
    RecvForwardedResult,
    AddTask,
    ProcessTask
};

struct TimeData
{
    OffloadPosition m_pos;

    uint64_t m_offloadPointId;

    uint64_t m_sendRequest;
    uint64_t m_recvResult;

    uint64_t m_recvRequest;
    uint64_t m_sendResult;

    uint64_t m_recvRequestToBeForwarded;
    uint64_t m_recvForwardedResult;

    uint64_t m_addTask;
    uint64_t m_processTask;

    TimeData()
        : m_pos(LocalGnb),
          m_offloadPointId(0),
          m_sendRequest(0),
          m_recvResult(0),
          m_recvRequest(0),
          m_sendResult(0),
          m_recvRequestToBeForwarded(0),
          m_recvForwardedResult(0),
          m_addTask(0),
          m_processTask(0)
    {
    }
};

typedef std::map<UeTaskIdPair_t, TimeData> TimeDataBuffer;

class CfTimeBuffer : public Object
{
  public:
    CfTimeBuffer();

    virtual ~CfTimeBuffer();

    static TypeId GetTypeId();

    void DoDispose() override;

    static void CfTimeBufferCallback(Ptr<CfTimeBuffer> buffer,
                                     uint64_t ueId,
                                     uint64_t taskId,
                                     uint64_t time,
                                     TimeType type,
                                     OffloadPosition pos = None);

    void UpdateTimeBuffer(uint64_t ueId,
                          uint64_t taskId,
                          uint64_t time,
                          TimeType type,
                          OffloadPosition pos = None);
    
    bool CheckTimeData(uint64_t ueId, uint64_t taskId);

    TimeData GetTimeData(uint64_t ueId, uint64_t taskId);

    void RemoveTimeData(uint64_t ueId, uint64_t taskId);

  private:
    TimeDataBuffer m_timeDataBuffer;
    // static void UeRecvResultCallback(Ptr<CfTimeBuffer> buffer,
    //                                   uint64_t ueId,
    //                                   uint64_t taskId,
    //                                   uint64_t time);
    // static void RecvRequestCallback(Ptr<CfTimeBuffer> buffer,
    //                                   uint64_t ueId,
    //                                   uint64_t taskId,
    //                                   uint64_t time);
    // static void SendResultsCallback(Ptr<CfTimeBuffer> buffer,
    //                                   uint64_t ueId,
    //                                   uint64_t taskId,
    //                                   uint64_t time);
    // static void GnbRecvRequestToBeForwardedCallback(Ptr<CfTimeBuffer> buffer,
    //                                   uint64_t ueId,
    //                                   uint64_t taskId,
    //                                   uint64_t time);
    // static void GnbRecvForwardedResultCallback(Ptr<CfTimeBuffer> buffer,
    //                                   uint64_t ueId,
    //                                   uint64_t taskId,
    //                                   uint64_t time);
    // static void CfAppAddTaskCallback(Ptr<CfTimeBuffer> buffer,
    //                                   uint64_t ueId,
    //                                   uint64_t taskId,
    //                                   uint64_t time);
    // static void CfUnitProcessTaskCallback(Ptr<CfTimeBuffer> buffer,
    //                                   uint64_t ueId,
    //                                   uint64_t taskId,
    //                                   uint64_t time);
    // static void UeSendRequestCallback(Ptr<CfTimeBuffer> buffer,
    //                                   uint64_t ueId,
    //                                   uint64_t taskId,
    //                                   uint64_t time);
};

} // namespace ns3

#endif