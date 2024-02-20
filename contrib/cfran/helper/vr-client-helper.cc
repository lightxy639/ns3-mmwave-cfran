#include "vr-client-helper.h"

#include <ns3/log.h>
#include <ns3/mc-ue-net-device.h>
#include <ns3/vr-client.h>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("VrClientHelper");

NS_OBJECT_ENSURE_REGISTERED(VrClientHelper);

VrClientHelper::VrClientHelper()
{
    NS_LOG_FUNCTION(this);
}

VrClientHelper::~VrClientHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId
VrClientHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::VrClientHelper").SetParent<Object>().AddConstructor<VrClientHelper>();
    return tid;
}

void
NonIpPacketRx(Ptr<NetDevice> netDev, Ptr<Packet> packet)
{
    Ptr<Node> node = netDev->GetNode();
    uint32_t nApp = node->GetNApplications();
    for (uint32_t index = 0; index < nApp; index++)
    {
        Ptr<VrClient> app = DynamicCast<VrClient>(node->GetApplication(index));
        if (app)
        {
            // std::cout << "OK" << std::endl;
            app->HandlePacket(packet);
            break;
        }
    }
}

ApplicationContainer
VrClientHelper::Install(NodeContainer c)
{
    NS_LOG_FUNCTION(this);
    ApplicationContainer apps;

    for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i)
    {
        Ptr<Node> node = *i;
        Ptr<Application> app = CreateObject<VrClient>();

        node->AddApplication(app);
        Ptr<mmwave::McUeNetDevice> netDev = DynamicCast<mmwave::McUeNetDevice>(node->GetDevice(0));
        NS_ASSERT(netDev != nullptr);
        netDev->SetNonIpReceiveCallback(MakeCallback(&NonIpPacketRx));

        apps.Add(app);
    }
    
    return apps;
}

} // namespace ns3