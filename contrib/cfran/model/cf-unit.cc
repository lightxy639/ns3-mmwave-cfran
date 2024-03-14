#include "cf-unit.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("CfUnit");

NS_OBJECT_ENSURE_REGISTERED(CfUnit);

TypeId
CfUnit::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CfUnit").SetParent<Object>();

    return tid;
}

void
CfUnit::DoDispose()
{
    NS_LOG_FUNCTION(this);
    Object::DoDispose();
}

CfUnit::CfUnit()
{
    NS_LOG_FUNCTION(this);
    CfModel cf("GPU", 82.6);
    m_cf = cf;
}

CfUnit::CfUnit(CfModel cf)
    : m_cf(cf)
{
    NS_LOG_FUNCTION(this);
}

CfUnit::~CfUnit()
{
    NS_LOG_FUNCTION(this);
}

void
CfUnit::SetNode(Ptr<Node> node)
{
    NS_LOG_FUNCTION(this << node);
    m_node = node;
}

void
CfUnit::SetCfApplication(Ptr<CfApplication> cfApp)
{
    m_cfApplication = cfApp;
}

} // namespace ns3