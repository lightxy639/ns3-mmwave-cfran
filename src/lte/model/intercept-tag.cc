#include "intercept-tag.h"

#include <ns3/tag.h>
#include <ns3/uinteger.h>

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(InterceptTag);

InterceptTag::InterceptTag()
    : m_intercept(true)
{
}

InterceptTag::InterceptTag(bool intercept)
    : m_intercept(intercept)
{
}

TypeId
InterceptTag::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::InterceptTag")
                            .SetParent<Tag>()
                            .SetGroupName("Lte")
                            .AddConstructor<InterceptTag>();

    return tid;
}

TypeId
InterceptTag::GetInstanceTypeId(void) const
{
    return GetTypeId();
}

uint32_t
InterceptTag::GetSerializedSize(void) const
{
    return sizeof(m_intercept);
}

void
InterceptTag::Serialize(TagBuffer i) const
{
    // i.Write((const uint8_t*)&m_intercept, sizeof(bool));
    i.WriteU8(m_intercept);
}

void
InterceptTag::Deserialize(TagBuffer i)
{
    m_intercept = i.ReadU8();
}

void
InterceptTag::Print(std::ostream& os) const
{
    os << "Intercept Tag" << m_intercept;
}

} // namespace ns3
