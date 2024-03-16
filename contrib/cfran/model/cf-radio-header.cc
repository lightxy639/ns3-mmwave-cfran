#include "cf-radio-header.h"

#include <ns3/log.h>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("CfRadioHeader");
NS_OBJECT_ENSURE_REGISTERED(CfRadioHeader);

CfRadioHeader::CfRadioHeader()
    : m_ueId(0),
      m_taskId(0),
      m_gnbId(0)
{
}

CfRadioHeader::~CfRadioHeader()
{
}

TypeId
CfRadioHeader::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::CfRadioHeader").SetParent<Header>().AddConstructor<CfRadioHeader>();

    return tid;
}

TypeId
CfRadioHeader::GetInstanceTypeId(void) const
{
    return GetTypeId();
}

void
CfRadioHeader::SetMessageType(uint8_t type)
{
    m_messageType = type;
}

void
CfRadioHeader::SetGnbId(uint64_t gnbId)
{
    m_gnbId = gnbId;
}

void
CfRadioHeader::SetUeId(uint64_t ueId)
{
    m_ueId = ueId;
}

void
CfRadioHeader::SetTaskId(uint64_t taskId)
{
    m_taskId = taskId;
}

uint8_t
CfRadioHeader::GetMessageType()
{
    return m_messageType;
}

uint64_t
CfRadioHeader::GetUeId()
{
    return m_ueId;
}

uint64_t
CfRadioHeader::GetGnbId()
{
    return m_gnbId;
}

uint64_t
CfRadioHeader::GetTaskId()
{
    return m_taskId;
}

void
CfRadioHeader::Print(std::ostream& os) const
{
    os << "MessageType " << m_messageType;
    os << "gnbId" << m_gnbId;
    os << "ueId " << m_ueId;
    os << "taskId " << m_taskId;
    return;
}

uint32_t
CfRadioHeader::GetSerializedSize() const
{
    return sizeof(m_messageType) + sizeof(m_gnbId) + sizeof(m_ueId) + sizeof(m_taskId);
}

void
CfRadioHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU8(m_messageType);
    i.WriteU64(m_gnbId);
    i.WriteU64(m_ueId);
    i.WriteU64(m_taskId);
}

uint32_t
CfRadioHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_messageType = i.ReadU8();
    m_gnbId = i.ReadU64();
    m_ueId = i.ReadU64();
    m_taskId = i.ReadU64();

    return GetSerializedSize();
}

} // namespace ns3