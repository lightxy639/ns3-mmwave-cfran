#include "cf-application-helper.h"

#include <ns3/log.h>
#include <ns3/cf-application.h>
// #include <ns3/mmwave-enb-net-device.h>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("CfApplicationHelper");

NS_OBJECT_ENSURE_REGISTERED(CfApplicationHelper);

CfApplicationHelper::CfApplicationHelper()
{
    NS_LOG_FUNCTION(this);
}

CfApplicationHelper::~CfApplicationHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId
CfApplicationHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::CfApplicationHelper").SetParent<Object>().AddConstructor<CfApplicationHelper>();
    return tid;
}

ApplicationContainer
CfApplicationHelper::Install(NodeContainer c)
{
    NS_LOG_FUNCTION(this);
    ApplicationContainer apps;

    for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i)
    {
        Ptr<Node> node = *i;
        Ptr<Application> app = CreateObject<CfApplication>();

        node->AddApplication(app);

        apps.Add(app);

        // for (uint32_t n = 0; n < node->GetNDevices(); n++)
        // {
        //     Ptr<NetDevice> netDev = node->GetDevice(n);

        //     Ptr<mmwave::MmWaveEnbNetDevice> mmWaveEnbNetDev = DynamicCast<mmwave::MmWaveEnbNetDevice>(netDev);

        //     if(mmWaveEnbNetDev)
        //     {
        //         DynamicCast<CfApplication>(app)->SetMmWaveEnbNetDevice(mmWaveEnbNetDev);
        //     }
        // }
    }
    
    return apps;
}
}