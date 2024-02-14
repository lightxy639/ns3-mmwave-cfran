#include "cfran-helper.h"
#include <ns3/object-factory.h>

namespace ns3
{

/* ... */
NS_LOG_COMPONENT_DEFINE("CfRanHelper");

NS_OBJECT_ENSURE_REGISTERED(CfRanHelper);

CfRanHelper::CfRanHelper()
{
    NS_LOG_FUNCTION(this);
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

} // namespace ns3
