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

CfApplicationHelper::CfApplicationHelper() : m_remoteIdOffset(0)
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
                            .AddConstructor<CfApplicationHelper>()
                            .AddAttribute("E2ModeCfApp",
                                          "If true, enable reporting over E2 for NR cells.",
                                          BooleanValue(true),
                                          MakeBooleanAccessor(&CfApplicationHelper::m_e2ModeCfApp),
                                          MakeBooleanChecker())
                            .AddAttribute("E2TermIp",
                                          "The IP address of the RIC E2 termination",
                                          StringValue("10.244.0.240"),
                                          MakeStringAccessor(&CfApplicationHelper::m_e2ip),
                                          MakeStringChecker())
                            .AddAttribute("E2Port",
                                          "Port number for E2",
                                          UintegerValue(36422),
                                          MakeUintegerAccessor(&CfApplicationHelper::m_e2port),
                                          MakeUintegerChecker<uint16_t>())
                            .AddAttribute("E2LocalPort",
                                          "The first port number for the local bind",
                                          UintegerValue(38470),
                                          MakeUintegerAccessor(&CfApplicationHelper::m_e2localPort),
                                          MakeUintegerChecker<uint16_t>());
    return tid;
}

void
CfApplicationHelper::SetRemoteIdOffset(uint16_t remoteIdOffset)
{
    m_remoteIdOffset = remoteIdOffset;
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
            NS_ASSERT(cfUnit != nullptr);

            uint16_t serverId = m_remoteIdOffset + index;

            cfUnit->SetCfUnitId(serverId);
            // DynamicCast<GnbCfApplication>(app)->SetCfUnit(cfUnit);
            DynamicCast<RemoteCfApplication>(app)->SetAttribute("CfUnit", PointerValue(cfUnit));
            DynamicCast<RemoteCfApplication>(app)->SetServerId(serverId);
            cfUnit->SetCfApplication(DynamicCast<RemoteCfApplication>(app));

            if (m_e2ModeCfApp)
            {
                const uint16_t local_port = m_e2localPort + (uint16_t)serverId;
                const std::string gnb_id{std::to_string(serverId)};

                std::string plmnId = "111";
                Ptr<E2Termination> e2term =
                    CreateObject<E2Termination>(m_e2ip, m_e2port, local_port, gnb_id, plmnId);
                DynamicCast<RemoteCfApplication>(app)->SetE2Termination(e2term);
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

                    if (m_e2ModeCfApp && mmWaveEnbNetDev->GetE2Termination() != nullptr)
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