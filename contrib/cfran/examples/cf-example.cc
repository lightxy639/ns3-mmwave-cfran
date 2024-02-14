// #include "ns3/cfran-helper.h"
// #include "ns3/applications-module.h"
// #include "ns3/config-store.h"
// #include "ns3/core-module.h"
// #include "ns3/global-route-manager.h"
// #include "ns3/internet-module.h"
// #include "ns3/ipv4-global-routing-helper.h"
// #include "ns3/mmwave-helper.h"
// #include "ns3/mobility-module.h"
// #include "ns3/network-module.h"
// #include <ns3/buildings-helper.h>

#include "ns3/cf-unit.h"
#include "ns3/core-module.h"
// #include "ns3/config-store.h"
#include "ns3/log.h"
#include "ns3/node-container.h"
#include "ns3/cfran-helper.h"
#include "ns3/vr-server-helper.h"
#include "ns3/vr-server.h"
#include "ns3/system-info.h"

/**
 * \file
 *
 * Explain here what the example does.
 */

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("CfExample");

int
main(int argc, char* argv[])
{
    LogComponentEnable("CfExample", LOG_DEBUG);
    LogComponentEnable("CfUnit", LOG_DEBUG);
    LogComponentEnable("CfUnit", LOG_FUNCTION);
    LogComponentEnable("VrServer", LOG_DEBUG);

    bool verbose = true;

    CommandLine cmd(__FILE__);
    cmd.AddValue("verbose", "Tell application to log if true", verbose);

    cmd.Parse(argc, argv);

    Ptr<CfranSystemInfo> cfranSystemInfo = CreateObject<CfranSystemInfo>();
    for( uint64_t imsi = 1; imsi <=5; imsi++)
    {
        CfranSystemInfo::UeInfo ueInfo;
        UeTaskModel ueTaskModel;
        // ueTaskModel.m_cfRequired = CfModel("GPU", 10);
        ueTaskModel.m_cfLoad = 0.2;
        ueTaskModel.m_deadline = 10;

        ueInfo.m_imsi = imsi;
        ueInfo.m_taskModel = ueTaskModel;
        ueInfo.m_taskPeriodity = 16;

        cfranSystemInfo->AddUeInfo(imsi, ueInfo);
    }
    Config::SetDefault("ns3::VrServer::CfranSystemInfo", PointerValue(cfranSystemInfo));

    NodeContainer testNodes;
    testNodes.Create(1);

    ObjectFactory cfUnitObj;
    cfUnitObj.SetTypeId("ns3::CfUnit");
    CfModel cfModel("GPU", 82.6);
    cfUnitObj.Set("EnableAutoSchedule", BooleanValue(false));
    cfUnitObj.Set("CfModel", CfModelValue(cfModel));

    Ptr<CfRanHelper> cfRanHelper = CreateObject<CfRanHelper>();
    cfRanHelper->InstallCfUnit(testNodes, cfUnitObj);

    Ptr<VrServerHelper> vrServerHelper= Create<VrServerHelper>();
    ApplicationContainer apps = vrServerHelper->Install(testNodes);
    Ptr<CfApplication> vrApp = DynamicCast<CfApplication>(apps.Get(0));

    // CfModel cfRequired("GPU", 10);
    // UeTaskModel ueTask(1, cfRequired, 20, 10);
    // ueTask.m_taskId = 1;
    // ueTask.m_cfRequired = cfRequired;
    // ueTask.m_cfLoad = 20;
    // ueTask.m_deadline = 10;
    // ueTask.m_application = vrApp;

    // vrApp->LoadTaskToCfUnit(1, ueTask);

    DynamicCast<VrServer>(vrApp)->StartServiceForImsi(1);


    /* ... */
    // CfUnit cfUnitExample();
    // Config::SetDefault("ns3::CfUnit::EnableAutoSchedule", BooleanValue(false));
    // Ptr<CfUnit> cfUnitExample = CreateObject<CfUnit>();

    // cfUnitExample->SetAttribute("EnableAutoSchedule", BooleanValue(false));

    // CfModel cfRequired("GPU", 10);
    // UeTaskModel ueTask(1, cfRequired, 20, 10);

    Simulator::Stop(Seconds(10));
    // cfUnitExample->AddNewUeTaskForSchedule(1, ueTask);
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
