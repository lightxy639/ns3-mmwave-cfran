#include "vr-server-helper.h"

#include <ns3/vr-server.h>
#include <ns3/log.h>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("VrServerHelper");

NS_OBJECT_ENSURE_REGISTERED(VrServerHelper);

VrServerHelper::VrServerHelper()
{
    NS_LOG_FUNCTION(this);
}

VrServerHelper::~VrServerHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId
VrServerHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::VrServerHelper").SetParent<Object>().AddConstructor<VrServerHelper>();
    return tid;
}

ApplicationContainer
VrServerHelper::Install(NodeContainer c)
{
    NS_LOG_FUNCTION(this);
    ApplicationContainer apps;

    for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i)
    {
        Ptr<Application> app = CreateObject<VrServer>();

        (*i)->AddApplication(app);

        Ptr<CfUnit> cfUnit = (*i)->GetObject<CfUnit>();

        DynamicCast<CfApplication>(app)->SetCfUnit(cfUnit);

        apps.Add(app);

    }

    return apps;
}
} // namespace ns3