#include "cf-time-buffer.h"

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
    
}
} // namespace ns3