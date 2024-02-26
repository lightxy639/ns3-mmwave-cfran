#include "enb-server-tag.h"

#include "ns3/tag.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(EnbServerTag);

EnbServerTag::EnbServerTag()
    : m_flag(false)
{
    // Nothing to do here
}

EnbServerTag::EnbServerTag(bool flag)
    : m_flag(flag)
{
    // Nothing to do here
}

TypeId
EnbServerTag::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::EnbServerTag")
                            .SetParent<Tag>()
                            .SetGroupName("Lte")
                            .AddConstructor<EnbServerTag>();
    return tid;
}

TypeId
EnbServerTag::GetInstanceTypeId(void) const
{
    return GetTypeId();
}

uint32_t
EnbServerTag::GetSerializedSize(void) const
{
    return sizeof(bool);
}

void
EnbServerTag::Serialize(TagBuffer i) const
{
    i.Write((const uint8_t*)&m_flag, sizeof(bool));
}

void
EnbServerTag::Deserialize(TagBuffer i)
{
    bool flag;
    i.Read((uint8_t*)&flag, 1);
    m_flag = flag;
}

void
EnbServerTag::Print(std::ostream& os) const
{
    os << m_flag;
}

} // namespace ns3
