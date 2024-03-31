#include "cfran-helper.h"

#include <ns3/config.h>
#include <ns3/object-factory.h>

namespace ns3
{

/* ... */
NS_LOG_COMPONENT_DEFINE("CfRanHelper");

NS_OBJECT_ENSURE_REGISTERED(CfRanHelper);

CfRanHelper::CfRanHelper()
{
    NS_LOG_FUNCTION(this);

    m_cfE2eCalculator = Create<CfE2eCalculator>();
    m_cfE2eBuffer = Create<CfE2eBuffer>();
}

CfRanHelper::~CfRanHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId
CfRanHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::CfRanHelper").SetParent<Object>().AddConstructor<CfRanHelper>();
    return tid;
}

void
CfRanHelper::InstallCfUnit(NodeContainer c, ObjectFactory cfUnitObj)
{
    NS_LOG_FUNCTION(this);

    for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i)
    {
        Ptr<CfUnit> cfUnit = DynamicCast<CfUnit>(cfUnitObj.Create());
        (*i)->AggregateObject(cfUnit);
    }
}

void
CfRanHelper::EnableTraces()
{
    NS_LOG_FUNCTION(this);

    Config::ConnectWithoutContextFailSafe("/NodeList/*/ApplicationList/*/$ns3::UeCfApplication/TxRequest",
                            MakeBoundCallback(&CfE2eBuffer::TxRequestCallback, m_cfE2eBuffer));
}

} // namespace ns3
