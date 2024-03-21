#include "multi-packet-header.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("MultiPacketHeader");
NS_OBJECT_ENSURE_REGISTERED(MultiPacketHeader);

MultiPacketHeader::MultiPacketHeader()
    : m_packetId(0),
      m_totalpacketNum(0)
{
}

MultiPacketHeader::~MultiPacketHeader()
{
}

TypeId
MultiPacketHeader::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::MultiPacketHeader").SetParent<Header>().AddConstructor<MultiPacketHeader>();

    return tid;
}

TypeId
MultiPacketHeader::GetInstanceTypeId(void) const
{
    return GetTypeId();
}

void
MultiPacketHeader::SetPacketId(uint64_t packetId)
{
    m_packetId = packetId;
}

void
MultiPacketHeader::SetTotalpacketNum(uint64_t totalPacketNum)
{
    m_totalpacketNum = totalPacketNum;
}

uint64_t
MultiPacketHeader::GetPacketId()
{
    return m_packetId;
}

uint64_t
MultiPacketHeader::GetTotalPacketNum()
{
    return m_totalpacketNum;
}

void
MultiPacketHeader::Print(std::ostream& os) const
{
    os << "packetId " << m_packetId;
    os << "totalPacketNum " << m_totalpacketNum;

    return;
}

uint32_t
MultiPacketHeader::GetSerializedSize() const
{
    return sizeof(m_packetId) + sizeof(m_totalpacketNum);
}

void
MultiPacketHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU64(m_packetId);
    i.WriteU64(m_totalpacketNum);
}

uint32_t
MultiPacketHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    
    m_packetId = i.ReadU64();
    m_totalpacketNum = i.ReadU64();

    return GetSerializedSize();
}
} // namespace ns3
