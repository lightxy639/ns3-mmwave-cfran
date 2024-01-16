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
    
    bool verbose = true;

    CommandLine cmd(__FILE__);
    cmd.AddValue("verbose", "Tell application to log if true", verbose);

    cmd.Parse(argc, argv);

    /* ... */
    // CfUnit cfUnitExample();
    CfModel cfModel("GPU", 50);
    // Config::SetDefault("ns3::CfUnit::EnableAutoSchedule", BooleanValue(false));
    Ptr<CfUnit> cfUnitExample = CreateObject<CfUnit>();
    
    cfUnitExample->SetAttribute("EnableAutoSchedule", BooleanValue(false));
    
    CfModel cfRequired("GPU", 10);
    UeTaskModel ueTask(1, cfRequired, 20, 10);
    Simulator::Stop(Seconds(10));
    cfUnitExample->AddNewUeTaskForSchedule(1, ueTask);
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
