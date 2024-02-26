#ifndef ENB_SERVER_TAG_H
#define ENB_SERVER_TAG_H

#include "ns3/packet.h"

namespace ns3
{
class Tag;

/**
 * Tag to indicate whehther the packet is being processed by a custom application on the
 * enb
 */

class EnbServerTag : public Tag
{
  public:
    static TypeId GetTypeId(void);
    virtual TypeId GetInstanceTypeId(void) const;

    /**
     * Create an empty X2 tag
     */
    EnbServerTag();
    /**
     * Create an X2 tag with the given flag
     */
    EnbServerTag(bool flag);

    virtual void Serialize(TagBuffer i) const;
    virtual void Deserialize(TagBuffer i);
    virtual uint32_t GetSerializedSize() const;
    virtual void Print(std::ostream& os) const;

    /**
     * Get the flag which indicate whehther the packet is being processed by a custom application on the
     * enb
     * @return the flag
     */
    bool GetFlag(void) const
    {
        return m_flag;
    }

    /**
     * Set the flag which indicate whehther the packet is being processed by a custom application on the
     * @param senderTimestamp time stamp of the instant when the X2 delivers the PDU
     */
    void SetSenderTimestamp(bool flag)
    {
        this->m_flag = flag;
    }

  private:
    bool m_flag;
};
} // namespace ns3
#endif