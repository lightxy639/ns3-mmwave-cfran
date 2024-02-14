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
CfranSystemInfo::GetCellInfo(uint64_t imsi)
{
    auto it = m_cellInfo.find(imsi);
    NS_ASSERT(it != m_cellInfo.end());

    return it->second;
}
} // namespace ns3
