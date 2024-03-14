#ifndef CHEAT_TAG_H
#define CHEAT_TAG_H

#include "ns3/packet.h"

namespace ns3
{
namespace mmwave
{
class CheatTag : public Tag
{
  public:
    static TypeId GetTypeId(void);
    virtual TypeId GetInstanceTypeId(void) const;

    CheatTag(uint32_t gtpTeid);
};
} // namespace mmwave
} // namespace ns3
#endif