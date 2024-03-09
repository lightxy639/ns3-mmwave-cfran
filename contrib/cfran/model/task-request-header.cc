#include "task-request-header.h"

#include <ns3/log.h>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("TaskRequestHeader");

NS_OBJECT_ENSURE_REGISTERED(TaskRequestHeader);

TaskRequestHeader::TaskRequestHeader()
    : m_ueId(0),
      m_taskId(0)
{
}

TaskRequestHeader::~TaskRequestHeader()
{
}

TypeId
TaskRequestHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TaskRequestHeader")
                        .SetParent<Header>()
                        .AddConstructor<TaskRequestHeader>();
    
    return tid;
}

TypeId
TaskRequestHeader::GetInstanceTypeId(void) const
{
    return GetTypeId();
}

void
TaskRequestHeader::SetUeId(uint64_t ueId)
{
    m_ueId = ueId;
}

void
TaskRequestHeader::SetTaskId(uint64_t taskId)
{
    m_taskId = taskId;
}

void
TaskRequestHeader::Print(std::ostream& os) const
{
    os << "ueId " << m_ueId;
    os << "taskId " << m_taskId;
    return;
}

uint32_t
TaskRequestHeader::GetSerializedSize() const
{
    return sizeof(m_ueId) + sizeof(m_taskId);
}

void
TaskRequestHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;

    i.WriteU64(m_ueId);
    i.WriteU64(m_taskId);
}

uint32_t
TaskRequestHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_ueId = i.ReadU64();
    m_taskId = i.ReadU64();

    return GetSerializedSize();
}
} // namespace ns3