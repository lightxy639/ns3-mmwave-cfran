#ifndef CF_RADIO_HEADER_H
#define CF_RADIO_HEADER_H

#include <ns3/header.h>

namespace ns3
{

/**
 * \ingroup cfran
 * \brief The header used for UE <-> gNB in cfran.
 */

class CfRadioHeader : public Header
{
  public:
    /**
     * \brief Constructor
     *
     * Creates a null header
     */
    enum MessageType
    {
        InitRequest = 0,
        InitSuccess = 1,
        TaskRequest = 2,
        TaskResult = 3,
        OffloadCommand = 4,
        TerminateCommand = 5
    };

    CfRadioHeader();

    ~CfRadioHeader();

    void SetMessageType(uint8_t type);
    void SetUeId(uint64_t ueId);
    void SetTaskId(uint64_t taskId);
    void SetGnbId(uint64_t gnbId);


    uint8_t GetMessageType();
    uint64_t GetUeId();
    uint64_t GetTaskId();
    uint64_t GetGnbId();
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

  protected:
    uint8_t m_messageType;
    uint64_t m_ueId;
    uint64_t m_taskId;
    uint64_t m_gnbId;
};
} // namespace ns3
#endif