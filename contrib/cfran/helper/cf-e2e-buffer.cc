#include "cf-e2e-buffer.h"

#include <ns3/config.h>
#include <ns3/log.h>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("CfE2eBuffer");
NS_OBJECT_ENSURE_REGISTERED(CfE2eBuffer);

CfE2eBuffer::CfE2eBuffer()
{
    NS_LOG_FUNCTION(this);
}

TypeId
CfE2eBuffer::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::CfE2eBuffer").SetParent<Object>().AddConstructor<CfE2eBuffer>();
    return tid;
}

void
CfE2eBuffer::DoDispose()
{
    NS_LOG_FUNCTION(this);
}

CfE2eBuffer::~CfE2eBuffer()
{
    NS_LOG_FUNCTION(this);
}

uint64_t
CfE2eBuffer::GetSpecificTimeBuffer(uint64_t ueId,
                                   uint64_t taskId,
                                   DetailedTimeStatsBuffer& timeBuffer,
                                   bool erase)
{
    UeTaskIdPair_t p(ueId, taskId);
    auto it = timeBuffer.find(p);

    if (it != timeBuffer.end())
    {
        uint64_t time = it->second;
        if (erase)
        {
            timeBuffer.erase(it);
        }
        return time;
    }
    else
    {
        NS_FATAL_ERROR("Detailed time for " << ueId << " not found");
    }
}

void
CfE2eBuffer::UpdateSpecificTimeBuffer(uint64_t ueId,
                                      uint64_t taskId,
                                      uint64_t time,
                                      DetailedTimeStatsBuffer& timeBuffer)
{
    UeTaskIdPair_t p(ueId, taskId);
    auto it = timeBuffer.find(p);

    if (it == timeBuffer.end())
    {
        timeBuffer[p] = time;
    }
    else
    {
        NS_FATAL_ERROR("Time info exists,");
    }
}

void
CfE2eBuffer::UpdateUplinkTimeBuffer(uint64_t ueId, uint64_t taskId, uint64_t time)
{
    UpdateSpecificTimeBuffer(ueId, taskId, time, m_uplinkTime);
}

void
CfE2eBuffer::UpdateQueueTimeBuffer(uint64_t ueId, uint64_t taskId, uint64_t time)
{
    UpdateSpecificTimeBuffer(ueId, taskId, time, m_queueTime);
}

void
CfE2eBuffer::UpdateComputingTimeBuffer(uint64_t ueId, uint64_t taskId, uint64_t time)
{
    UpdateSpecificTimeBuffer(ueId, taskId, time, m_computingTime);
}

void
CfE2eBuffer::UpdateDownlinkTimeBuffer(uint64_t ueId, uint64_t taskId, uint64_t time)
{
    UpdateSpecificTimeBuffer(ueId, taskId, time, m_downlinkTime);
}

void
CfE2eBuffer::GetUplinkTimeBuffer(uint64_t ueId, uint64_t taskId, bool erase)
{
    GetSpecificTimeBuffer(ueId, taskId, m_uplinkTime, erase);
}

void
CfE2eBuffer::GetQueueTimeBuffer(uint64_t ueId, uint64_t taskId, bool erase)
{
    GetSpecificTimeBuffer(ueId, taskId, m_queueTime, erase);
}

void
CfE2eBuffer::GetComputingTimeBuffer(uint64_t ueId, uint64_t taskId, bool erase)
{
    GetSpecificTimeBuffer(ueId, taskId, m_computingTime, erase);
}

void
CfE2eBuffer::GetDownlinkTimeBuffer(uint64_t ueId, uint64_t taskId, bool erase)
{
    GetSpecificTimeBuffer(ueId, taskId, m_downlinkTime, erase);
}

void
CfE2eBuffer::TxRequestCallback(Ptr<CfE2eBuffer> stats, uint64_t ueId, uint64_t taskId, uint64_t time)
{
    NS_LOG_DEBUG(ueId << taskId << time);
    std::cout << "Hello" << std::endl;
    stats->UpdateUplinkTimeBuffer(ueId, taskId, time);
}

void
CfE2eBuffer::QueueCallback(Ptr<CfE2eBuffer> stats, uint64_t ueId, uint64_t taskId, uint64_t time)
{
    stats->UpdateQueueTimeBuffer(ueId, taskId, time);
}

void
CfE2eBuffer::ComputingCallback(Ptr<CfE2eBuffer> stats, uint64_t ueId, uint64_t taskId, uint64_t time)
{
    stats->UpdateComputingTimeBuffer(ueId, taskId, time);
}

void
CfE2eBuffer::TxResultCallback(Ptr<CfE2eBuffer> stats, uint64_t ueId, uint64_t taskId, uint64_t time)
{
    stats->UpdateDownlinkTimeBuffer(ueId, taskId, time);
}

} // namespace ns3