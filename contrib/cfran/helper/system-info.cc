#include "system-info.h"

#include <ns3/log.h>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("CfranSystemInfo");

NS_OBJECT_ENSURE_REGISTERED(CfranSystemInfo);

TypeId
CfranSystemInfo::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::CfranSystemInfo").SetParent<Object>().AddConstructor<CfranSystemInfo>();

    return tid;
}

CfranSystemInfo::CfranSystemInfo()
{
    NS_LOG_FUNCTION(this);
}

CfranSystemInfo::~CfranSystemInfo()
{
    NS_LOG_FUNCTION(this);
}

void
CfranSystemInfo::DoInitialize()
{
    NS_LOG_FUNCTION(this);
}

CfranSystemInfo::UeInfo
CfranSystemInfo::GetUeInfo(uint64_t imsi)
{
    auto it = m_ueInfo.find(imsi);
    NS_ASSERT(it != m_ueInfo.end());

    return it->second;
}

CfranSystemInfo::CellInfo
CfranSystemInfo::GetCellInfo(uint64_t cellId)
{
    auto it = m_cellInfo.find(cellId);
    NS_ASSERT(it != m_cellInfo.end());

    return it->second;
}

CfranSystemInfo::RemoteInfo
CfranSystemInfo::GetRemoteInfo(uint64_t remoteId)
{
    auto it = m_remoteInfo.find(remoteId);
    NS_ASSERT(it != m_remoteInfo.end());

    return it->second;
}

CfranSystemInfo::OffloadPointType
CfranSystemInfo::GetOffladPointType(uint64_t id)
{
    auto it_gnb = m_cellInfo.find(id);
    auto it_remote = m_remoteInfo.find(id);

    if(it_gnb != m_cellInfo.end())
    {
        return OffloadPointType::Gnb;
    }
    else if(it_remote != m_remoteInfo.end())
    {
        return OffloadPointType::Remote;
    }
}

void
CfranSystemInfo::AddUeInfo(uint64_t imsi, UeInfo ueInfo)
{
    m_ueInfo.insert(std::pair<uint64_t, UeInfo>(imsi, ueInfo));
}

void
CfranSystemInfo::AddCellInfo(uint64_t cellId, CellInfo cellInfo)
{
    m_cellInfo.insert(std::pair<uint64_t, CellInfo>(cellId, cellInfo));
}

void
CfranSystemInfo::AddRemoteInfo(uint64_t remoteId, RemoteInfo remoteInfo)
{
    NS_LOG_FUNCTION(this << remoteId);
    m_remoteInfo.insert(std::pair<uint64_t, RemoteInfo>(remoteId, remoteInfo));
}
} // namespace ns3
