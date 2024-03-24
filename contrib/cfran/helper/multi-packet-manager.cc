#include "multi-packet-manager.h"

#include <ns3/log.h>

#include <iostream>


namespace ns3
{
NS_LOG_COMPONENT_DEFINE("MultiPacketManager");

NS_OBJECT_ENSURE_REGISTERED(MultiPacketManager);

TypeId
MultiPacketManager::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::MultiPacketManager").SetParent<Object>().AddConstructor<MultiPacketManager>();

    return tid;
}

MultiPacketManager::MultiPacketManager()
    : m_recvQueueSize(5)
{
    NS_LOG_FUNCTION(this);
}

MultiPacketManager::~MultiPacketManager()
{
    NS_LOG_FUNCTION(this);
}

void
MultiPacketManager::DoInitialize()
{
    NS_LOG_FUNCTION(this);
}

bool
MultiPacketManager::AddAndCheckPacket(uint64_t sourceId,
                                      uint64_t fileId,
                                      uint64_t packetId,
                                      uint64_t totalPacketNum)
{
    NS_LOG_FUNCTION(this << sourceId << fileId << packetId << totalPacketNum);

    auto itSrc = m_fileInfo.find(sourceId);
    if (itSrc != m_fileInfo.end())
    {
        auto itFile = itSrc->second.find(fileId);
        if (itFile == itSrc->second.end())
        {
            // Add new file info
            m_fileInfo[sourceId][fileId] = FileInfo(totalPacketNum);
            m_recvControl[sourceId].push(fileId);
            NS_LOG_DEBUG("Add new (UE,file) " << sourceId << ", " << fileId << " Queue size: " << m_recvControl[sourceId].size());

            if (m_recvControl[sourceId].size() > m_recvQueueSize)
            {
                // Ensure that the size of file queue is fixed
                uint64_t popFileId = m_recvControl[sourceId].front();
                m_recvControl[sourceId].pop();
                m_fileInfo[sourceId].erase(popFileId);
                NS_LOG_DEBUG("The queue length has reached its maximum value, pop " << popFileId);
            }
        }
        else
        {
            itSrc->second[fileId].m_recviedPacketNum++;
            // NS_LOG_DEBUG("Increase value in map to " << itSrc->second[fileId].m_recviedPacketNum);
        }
    }
    else
    {
        NS_LOG_DEBUG("Add new source " << sourceId);
        m_fileInfo[sourceId][fileId] = FileInfo(totalPacketNum);
    }

    FileInfo fileInfo = m_fileInfo[sourceId][fileId];

    if (fileInfo.m_completed == false &&
        fileInfo.m_recviedPacketNum >= fileInfo.m_totalPacketNum * 0.9)
    {
        NS_LOG_DEBUG("Mininum number of packets " << "("  << sourceId << ", " << fileId << ")");
        m_fileInfo[sourceId][fileId].m_completed = true;
        return true;
    }
    else
    {
        return false;
    }
}
} // namespace ns3