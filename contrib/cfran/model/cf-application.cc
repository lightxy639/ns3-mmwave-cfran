#include "cf-application.h"

#include <ns3/log.h>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("CfApplication");

NS_OBJECT_ENSURE_REGISTERED(CfApplication);

TypeId
CfApplication::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CfApplication").SetParent<Application>();

    return tid;
}

CfApplication::CfApplication()
{
    NS_LOG_FUNCTION(this);
}

CfApplication::~CfApplication()
{
    NS_LOG_FUNCTION(this);
}

void
CfApplication::SetCfUnit(Ptr<CfUnit> cfUnit)
{
    m_cfUnit = cfUnit;
}

} // namespace ns3
