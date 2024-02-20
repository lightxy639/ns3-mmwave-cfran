#include "vr-client.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("VrClient");

NS_OBJECT_ENSURE_REGISTERED(VrClient);

TypeId
VrClient::GetTypeId()
{
    static TypeId tid = TypeId("ns3::VrClient").SetParent<Application>().AddConstructor<VrClient>();
    return tid;
}

VrClient::VrClient()
{
    NS_LOG_FUNCTION(this);
}

VrClient::~VrClient()
{
    NS_LOG_FUNCTION(this);
}

void
VrClient::HandlePacket(Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this);

    NS_LOG_DEBUG("VR packet arrived.");
}

void
VrClient::DoDispose()
{
    NS_LOG_FUNCTION(this);
    Application::DoDispose();
}

void
VrClient::StartApplication()
{
    NS_LOG_FUNCTION(this);
}

void
VrClient::StopApplication()
{
    NS_LOG_FUNCTION(this);
}
} // namespace ns3