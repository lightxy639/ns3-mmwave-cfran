#include "cf-x2-header.h"

#include <ns3/log.h>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("CfX2Header");
NS_OBJECT_ENSURE_REGISTERED(CfX2Header);

CfX2Header::CfX2Header()
    : m_ueId(0),
      m_taskId(0),
      m_sourceGnbId(0),
      m_targetGnbId(0)
{
}

CfX2Header::~CfX2Header()
{
}

TypeId
CfX2Header::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CfX2Header").SetParent<Header>().AddConstructor<CfX2Header>();

    return tid;
}

TypeId
CfX2Header::GetInstanceTypeId(void) const
{
    return GetTypeId();
}

void
CfX2Header::SetMessageType(uint8_t type)
{
    m_messageType = type;
}

void
CfX2Header::SetSourceGnbId(uint64_t gnbId)
{
    m_sourceGnbId = gnbId;
}

void
CfX2Header::SetTargetGnbId(uint64_t gnbId)
{
    m_targetGnbId = gnbId;
}

void
CfX2Header::SetUeId(uint64_t ueId)
{
    m_ueId = ueId;
}

void
CfX2Header::SetTaskId(uint64_t taskId)
{
    m_taskId = taskId;
}

uint8_t
CfX2Header::GetMessageType()
{
    return m_messageType;
}

uint64_t
CfX2Header::GetUeId()
{
    return m_ueId;
}

uint64_t
CfX2Header::GetSourceGnbId()
{
    return m_sourceGnbId;
}

uint64_t
CfX2Header::GetTargetGnbId()
{
    return m_targetGnbId;
}
uint64_t
CfX2Header::GetTaskId()
{
    return m_taskId;
}

void
CfX2Header::Print(std::ostream& os) const
{
    os << "messageType " << m_messageType;
    os << "sourceGnbId" << m_sourceGnbId;
    os << "targerGnbId" << m_targetGnbId;
    os << "ueId " << m_ueId;
    os << "taskId " << m_taskId;
    return;
}

uint32_t
CfX2Header::GetSerializedSize() const
{
    return sizeof(m_messageType) + sizeof(m_sourceGnbId) + sizeof(m_targetGnbId) + sizeof(m_ueId) + sizeof(m_taskId);
}

void
CfX2Header::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU8(m_messageType);
    i.WriteU64(m_sourceGnbId);
    i.WriteU64(m_targetGnbId);
    i.WriteU64(m_ueId);
    i.WriteU64(m_taskId);
}

uint32_t
CfX2Header::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_messageType = i.ReadU8();
    m_sourceGnbId = i.ReadU64();
    m_targetGnbId = i.ReadU64();
    m_ueId = i.ReadU64();
    m_taskId = i.ReadU64();

    return GetSerializedSize();
}

} // namespace ns3