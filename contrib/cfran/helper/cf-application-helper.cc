#include "cf-application-helper.h"

#include <ns3/cf-application.h>
#include <ns3/gnb-cf-application.h>
#include <ns3/log.h>
#include <ns3/remote-cf-application.h>

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
    static TypeId tid = TypeId("ns3::CfApplicationHelper")
                            .SetParent<Object>()
                            .AddConstructor<CfApplicationHelper>();
    return tid;
}

ApplicationContainer
CfApplicationHelper::Install(NodeContainer c, bool isGnb)
{
    NS_LOG_FUNCTION(this);
    ApplicationContainer apps;

    uint64_t index = 1;
    for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i)
    {
        Ptr<Node> node = *i;

        if (!isGnb)
        {
            Ptr<Application> app = CreateObject<RemoteCfApplication>();

            node->AddApplication(app);

            apps.Add(app);

            Ptr<CfUnit> cfUnit = node->GetObject<CfUnit>();
            cfUnit->SetCfUnitId(100 + index);

            if (cfUnit)
            {
                // DynamicCast<GnbCfApplication>(app)->SetCfUnit(cfUnit);
                DynamicCast<RemoteCfApplication>(app)->SetAttribute("CfUnit", PointerValue(cfUnit));
                DynamicCast<RemoteCfApplication>(app)->SetServerId(100 + index);
                cfUnit->SetCfApplication(DynamicCast<RemoteCfApplication>(app));
            }
            else
            {
                NS_FATAL_ERROR("No available cfunit on node.");
            }
        }
        else
        {
            Ptr<Application> app = CreateObject<GnbCfApplication>();

            node->AddApplication(app);

            apps.Add(app);
            for (uint32_t n = 0; n < node->GetNDevices(); n++)
            {
                Ptr<NetDevice> netDev = node->GetDevice(n);

                Ptr<mmwave::MmWaveEnbNetDevice> mmWaveEnbNetDev =
                    DynamicCast<mmwave::MmWaveEnbNetDevice>(netDev);

                if (mmWaveEnbNetDev)
                {
                    DynamicCast<GnbCfApplication>(app)->SetMmWaveEnbNetDevice(mmWaveEnbNetDev);

                    NS_LOG_DEBUG("Enroll mmwaveEnbNetDevice " << mmWaveEnbNetDev->GetCellId()
                                                              << " to app.");

                    Ptr<CfUnit> cfUnit = node->GetObject<CfUnit>();
                    cfUnit->SetCfUnitId(mmWaveEnbNetDev->GetCellId());
                    if (cfUnit)
                    {
                        // DynamicCast<GnbCfApplication>(app)->SetCfUnit(cfUnit);
                        DynamicCast<GnbCfApplication>(app)->SetAttribute("CfUnit",
                                                                         PointerValue(cfUnit));
                        cfUnit->SetCfApplication(DynamicCast<GnbCfApplication>(app));
                    }
                    else
                    {
                        NS_FATAL_ERROR("No available cfunit on node.");
                    }

                    if (mmWaveEnbNetDev->GetE2Termination() != nullptr)
                    {
                        NS_LOG_DEBUG("CfApplicationHelper SetE2Termination");
                        DynamicCast<GnbCfApplication>(app)->SetE2Termination(
                            mmWaveEnbNetDev->GetE2Termination());
                    }
                    break;
                }
            }
        }
    
        index++;
    }

    return apps;
}
} // namespace ns3