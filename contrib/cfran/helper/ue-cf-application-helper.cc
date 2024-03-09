#include "ue-cf-application-helper.h"

#include <ns3/log.h>
#include <ns3/cf-application.h>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("UeCfApplicationHelper");

NS_OBJECT_ENSURE_REGISTERED(UeCfApplicationHelper);

UeCfApplicationHelper::UeCfApplicationHelper()
{
    NS_LOG_FUNCTION(this);
}

UeCfApplicationHelper::~UeCfApplicationHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId
UeCfApplicationHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::UeCfApplicationHelper").SetParent<Object>().AddConstructor<UeCfApplicationHelper>();
    return tid;
}

ApplicationContainer
UeCfApplicationHelper::Install(NodeContainer c)
{
    NS_LOG_FUNCTION(this);
    ApplicationContainer apps;

    for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i)
    {
        Ptr<Node> node = *i;
        Ptr<Application> app = CreateObject<UeCfApplication>();

        node->AddApplication(app);

        apps.Add(app);
    }
    
    return apps;
}
}