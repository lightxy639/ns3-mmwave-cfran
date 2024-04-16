#ifndef INTERCEPT_TAG_H
#define INTERCEPT_TAG_H

#include "ns3/packet.h"

namespace ns3
{
namespace mmwave
{
class InterceptTag : public Tag
{
  public:
    static TypeId GetTypeId(void);
    virtual TypeId GetInstanceTypeId(void) const;

    InterceptTag();

    virtual void Serialize(TagBuffer i) const;
    virtual void Deserialize(TagBuffer i);
    virtual uint32_t GetSerializedSize() const;
    virtual void Print(std::ostream& os) const;

  private:
    bool 
};
} // namespace mmwave
} // namespace ns3
#endif