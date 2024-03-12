#include "ue-cf-application-helper.h"

#include <ns3/cf-application.h>
#include <ns3/log.h>
#include <ns3/mc-ue-net-device.h>

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
    static TypeId tid = TypeId("ns3::UeCfApplicationHelper")
                            .SetParent<Object>()
                            .AddConstructor<UeCfApplicationHelper>();
    return tid;
}


void
CfNonIpPacketRx(Ptr<NetDevice> netDev, Ptr<Packet> packet)
{
    Ptr<Node> node = netDev->GetNode();
    uint32_t nApp = node->GetNApplications();
    for (uint32_t index = 0; index < nApp; index++)
    {
        NS_LOG_DEBUG("Finding UeCfApplication");
        Ptr<UeCfApplication> app = DynamicCast<UeCfApplication>(node->GetApplication(index));
        if (app)
        {
            // std::cout << "OK" << std::endl;
            app->HandlePacket(packet);
            break;
        }

    }
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
        DynamicCast<UeCfApplication>(app)->SetUeId(
            DynamicCast<ns3::mmwave::McUeNetDevice>(node->GetDevice(0))->GetImsi());

        Ptr<mmwave::McUeNetDevice> netDev = DynamicCast<mmwave::McUeNetDevice>(node->GetDevice(0));
        NS_ASSERT(netDev != nullptr);
        netDev->SetNonIpReceiveCallback(MakeCallback(&CfNonIpPacketRx));

        node->AddApplication(app);

        apps.Add(app);
    }

    return apps;
}



} // namespace ns3