#include "cf-time-buffer.h"

#include <ns3/assert.h>
#include <ns3/config.h>
#include <ns3/log.h>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("CfTimeBuffer");
NS_OBJECT_ENSURE_REGISTERED(CfTimeBuffer);

CfTimeBuffer::CfTimeBuffer()
{
    NS_LOG_FUNCTION(this);
}

TypeId
CfTimeBuffer::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::CfTimeBuffer").SetParent<Object>().AddConstructor<CfTimeBuffer>();
    return tid;
}

void
CfTimeBuffer::DoDispose()
{
    NS_LOG_FUNCTION(this);
}

CfTimeBuffer::~CfTimeBuffer()
{
    NS_LOG_FUNCTION(this);
}

void
CfTimeBuffer::CfTimeBufferCallback(Ptr<CfTimeBuffer> buffer,
                                   uint64_t ueId,
                                   uint64_t taskId,
                                   uint64_t time,
                                   TimeType type,
                                   OffloadPosition pos)
{
    buffer->UpdateTimeBuffer(ueId, taskId, time, type, pos);
}

void
CfTimeBuffer::UpdateTimeBuffer(uint64_t ueId,
                               uint64_t taskId,
                               uint64_t time,
                               TimeType type,
                               OffloadPosition pos)
{
    UeTaskIdPair_t p(ueId, taskId);
    auto it = m_timeDataBuffer.find(p);

    if (it == m_timeDataBuffer.end())
    {
        m_timeDataBuffer[p] = TimeData();
    }

    if (pos != None)
    {
        m_timeDataBuffer[p].m_pos = pos;
    }

    switch (time)
    {
    case SendRequest:
        m_timeDataBuffer[p].m_sendRequest = time;
        break;
    case RecvResult:
        m_timeDataBuffer[p].m_recvResult = time;
        break;
    case RecvRequest:
        m_timeDataBuffer[p].m_recvRequest = time;
        break;
    case SendResult:
        m_timeDataBuffer[p].m_sendResult = time;
        break;
    case RecvRequestToBeForwarded:
        m_timeDataBuffer[p].m_recvRequestToBeForwarded = time;
        break;
    case RecvForwardedResult:
        m_timeDataBuffer[p].m_recvForwardedResult = time;
        break;
    case AddTask:
        m_timeDataBuffer[p].m_addTask = time;
        break;
    case ProcessTask:
        m_timeDataBuffer[p].m_processTask = time;
        break;
    }
}

TimeData
CfTimeBuffer::GetTimeData(uint64_t ueId, uint64_t taskId)
{
    UeTaskIdPair_t p(ueId, taskId);
    auto it = m_timeDataBuffer.find(p);

    NS_ASSERT(it != m_timeDataBuffer.end());

    return it->second;
}

void
CfTimeBuffer::RemoveTimeData(uint64_t ueId, uint64_t taskId)
{
    UeTaskIdPair_t p(ueId, taskId);
    m_timeDataBuffer.erase(p);
}
} // namespace ns3