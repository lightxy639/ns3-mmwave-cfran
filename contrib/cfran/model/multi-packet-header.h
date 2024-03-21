#ifndef MULTI_PACKET_HEADER_H
#define MULTI_PACKET_HEADER_H

#include <ns3/header.h>

namespace ns3
{
/**
 * \ingroup cfran
 * \brief The header used in the case of multiple packets forming a single file.
 */

class MultiPacketHeader : public Header
{
  public:
    MultiPacketHeader();
    ~MultiPacketHeader();

    void SetPacketId(uint64_t packetId);
    void SetTotalpacketNum(uint64_t totalPacketNum);

    uint64_t GetPacketId();
    uint64_t GetTotalPacketNum();
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId(void);
    virtual TypeId GetInstanceTypeId(void) const;
    virtual void Print(std::ostream& os) const;
    virtual uint32_t GetSerializedSize(void) const;
    virtual void Serialize(Buffer::Iterator start) const;
    virtual uint32_t Deserialize(Buffer::Iterator start);

  private:
    uint64_t m_packetId;
    uint64_t m_totalpacketNum;
};

} // namespace ns3
#endif