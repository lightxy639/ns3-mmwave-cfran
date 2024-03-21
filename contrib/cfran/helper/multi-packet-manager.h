#ifndef MULTI_PACKET_MANAGER_H
#define MULTI_PACKET_MANAGER_H

#include <ns3/object.h>

#include <map>
#include <queue>

namespace ns3
{
/**
 * \ingroup cfran
 *
 * Store the basice infomation in the case of multiple packets forming a single file.
 */

class MultiPacketManager : public Object
{
  public:
    struct FileInfo
    {
        uint64_t m_totalPacketNum;
        uint64_t m_recviedPacketNum;
        bool m_completed;

        FileInfo()
            : m_totalPacketNum(0),
              m_recviedPacketNum(0),
              m_completed(false)
        {
        }

        FileInfo(uint64_t totalPacketNum, uint64_t recviedPacketNum = 1, bool completed = false)
            : m_totalPacketNum(totalPacketNum),
              m_recviedPacketNum(recviedPacketNum),
              m_completed(completed)
        {
        }
    };

    MultiPacketManager();

    virtual ~MultiPacketManager();

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    bool AddAndCheckPacket(uint64_t sourceId,
                           uint64_t fileId,
                           uint64_t packetId,
                           uint64_t totalPacketNum);

  protected:
    // inherited from Object
    virtual void DoInitialize(void);

  private:
    // source, file, FileInfo
    std::map<uint64_t, std::map<uint64_t, FileInfo>> m_fileInfo;
    // sourec, file
    std::map<uint64_t, std::queue<uint64_t>> m_recvControl;
    uint8_t m_recvQueueSize;
};
} // namespace ns3

#endif