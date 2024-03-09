#ifndef TASK_REQUEST_HEADER_H
#define TASK_REQUEST_HEADER_H

#include <ns3/header.h>

namespace ns3
{

/**
 * \ingroup cfran
 * \brief The header carrying task request information.
 */
class TaskRequestHeader : public Header
{
  public:
    /**
     * \brief Constructor
     *
     * Creates a null header
     */
    TaskRequestHeader();

    ~TaskRequestHeader();

    void SetUeId(uint64_t ueId);

    void SetTaskId(uint64_t taskId);

    uint64_t GetUeId();

    uint64_t GetTaskIfd();

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
    uint64_t m_ueId;

    uint64_t m_taskId;
};

} // namespace ns3
#endif