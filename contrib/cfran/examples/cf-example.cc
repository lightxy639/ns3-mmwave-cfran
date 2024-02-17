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
#include "ns3/cfran-helper.h"
#include "ns3/config-store.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/isotropic-antenna-model.h"
#include "ns3/log.h"
#include "ns3/lte-ue-net-device.h"
#include "ns3/mmwave-helper.h"
#include "ns3/mmwave-point-to-point-epc-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/node-container.h"
#include "ns3/node-list.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/system-info.h"
#include "ns3/vr-server-helper.h"
#include "ns3/vr-server.h"

#define BS_HEIGHT 25           // 基站天线高度
#define UT_HEIGHT 1.5          // UE 高度
#define MIN_BS_UT_DISTANCE 100 // UE 与基站的最小距离

/**
 * \file
 *
 * Explain here what the example does.
 */

using namespace ns3;
using namespace mmwave;

NS_LOG_COMPONENT_DEFINE("CfExample");

void
PrintGnuplottableUeListToFile(std::string filename)
{
    std::ofstream outFile;
    outFile.open(filename.c_str(), std::ios_base::out | std::ios_base::trunc);
    if (!outFile.is_open())
    {
        NS_LOG_ERROR("Can't open file " << filename);
        return;
    }
    for (NodeList::Iterator it = NodeList::Begin(); it != NodeList::End(); ++it)
    {
        Ptr<Node> node = *it;
        int nDevs = node->GetNDevices();
        for (int j = 0; j < nDevs; j++)
        {
            Ptr<LteUeNetDevice> uedev = node->GetDevice(j)->GetObject<LteUeNetDevice>();
            Ptr<MmWaveUeNetDevice> mmuedev = node->GetDevice(j)->GetObject<MmWaveUeNetDevice>();
            Ptr<McUeNetDevice> mcuedev = node->GetDevice(j)->GetObject<McUeNetDevice>();
            if (uedev)
            {
                Vector pos = node->GetObject<MobilityModel>()->GetPosition();
                outFile << "set label \"" << uedev->GetImsi() << "\" at " << pos.x << "," << pos.y
                        << " left font \"Helvetica,8\" textcolor rgb \"black\" front point pt 1 ps "
                           "0.3 lc rgb \"black\" offset 0,0"
                        << std::endl;
            }
            else if (mmuedev)
            {
                Vector pos = node->GetObject<MobilityModel>()->GetPosition();
                outFile << "set label \"" << mmuedev->GetImsi() << "\" at " << pos.x << "," << pos.y
                        << " left font \"Helvetica,8\" textcolor rgb \"black\" front point pt 1 ps "
                           "0.3 lc rgb \"black\" offset 0,0"
                        << std::endl;
            }
            else if (mcuedev)
            {
                Vector pos = node->GetObject<MobilityModel>()->GetPosition();
                outFile << "set label \"" << mcuedev->GetImsi() << "\" at " << pos.x << "," << pos.y
                        << " left font \"Helvetica,8\" textcolor rgb \"black\" front point pt 1 ps "
                           "0.3 lc rgb \"black\" offset 0,0"
                        << std::endl;
            }
        }
    }
}

void
PrintGnuplottableEnbListToFile(std::string filename)
{
    std::ofstream outFile;
    outFile.open(filename.c_str(), std::ios_base::out | std::ios_base::trunc);
    if (!outFile.is_open())
    {
        NS_LOG_ERROR("Can't open file " << filename);
        return;
    }
    for (NodeList::Iterator it = NodeList::Begin(); it != NodeList::End(); ++it)
    {
        Ptr<Node> node = *it;
        int nDevs = node->GetNDevices();
        for (int j = 0; j < nDevs; j++)
        {
            Ptr<LteEnbNetDevice> enbdev = node->GetDevice(j)->GetObject<LteEnbNetDevice>();
            Ptr<MmWaveEnbNetDevice> mmdev = node->GetDevice(j)->GetObject<MmWaveEnbNetDevice>();
            if (enbdev)
            {
                Vector pos = node->GetObject<MobilityModel>()->GetPosition();
                outFile << "set label \"" << enbdev->GetCellId() << "\" at " << pos.x << ","
                        << pos.y
                        << " left font \"Helvetica,8\" textcolor rgb \"blue\" front  point pt 4 ps "
                           "0.3 lc rgb \"blue\" offset 0,0"
                        << std::endl;
            }
            else if (mmdev)
            {
                Vector pos = node->GetObject<MobilityModel>()->GetPosition();
                outFile << "set label \"" << mmdev->GetCellId() << "\" at " << pos.x << "," << pos.y
                        << " left font \"Helvetica,8\" textcolor rgb \"red\" front  point pt 4 ps "
                           "0.3 lc rgb \"red\" offset 0,0"
                        << std::endl;
            }
        }
    }
}

void
TargetEnbTest()
{
    for (NodeList::Iterator it = NodeList::Begin(); it != NodeList::End(); ++it)
    {
        Ptr<Node> node = *it;
        int nDevs = node->GetNDevices();
        for (int j = 0; j < nDevs; j++)
        {
            Ptr<McUeNetDevice> mcuedev = node->GetDevice(j)->GetObject<McUeNetDevice>();
            if (mcuedev)
            {
                Ptr<MmWaveEnbNetDevice> mmWaveEnbNetDev = mcuedev->GetMmWaveTargetEnb();
                if (mmWaveEnbNetDev != nullptr)
                {
                    NS_LOG_DEBUG("GetMmWaveTargetEnb returns valid ptr.");
                }
                else
                {
                    NS_LOG_DEBUG("GetMmWaveTargetEnb returns nullptr.");
                }
            }
        }
    }
}

static ns3::GlobalValue g_bufferSize("bufferSize",
                                     "RLC tx buffer size (MB)",
                                     ns3::UintegerValue(20),
                                     ns3::MakeUintegerChecker<uint32_t>());
static ns3::GlobalValue g_x2Latency("x2Latency",
                                    "Latency on X2 interface (us)",
                                    ns3::DoubleValue(500),
                                    ns3::MakeDoubleChecker<double>());
static ns3::GlobalValue g_mmeLatency("mmeLatency",
                                     "Latency on MME interface (us)",
                                     ns3::DoubleValue(10000),
                                     ns3::MakeDoubleChecker<double>());
static ns3::GlobalValue g_mobileUeSpeed("mobileSpeed",
                                        "The speed of the UE (m/s)",
                                        ns3::DoubleValue(2),
                                        ns3::MakeDoubleChecker<double>());
static ns3::GlobalValue g_rlcAmEnabled("rlcAmEnabled",
                                       "If true, use RLC AM, else use RLC UM",
                                       ns3::BooleanValue(true),
                                       ns3::MakeBooleanChecker());
static ns3::GlobalValue g_outPath("outPath",
                                  "The path of output log files",
                                  ns3::StringValue("./"),
                                  ns3::MakeStringChecker());
static ns3::GlobalValue g_noiseAndFilter(
    "noiseAndFilter",
    "If true, use noisy SINR samples, filtered. If false, just use the SINR measure",
    ns3::BooleanValue(false),
    ns3::MakeBooleanChecker());
static ns3::GlobalValue g_handoverMode("handoverMode",
                                       "Handover mode",
                                       ns3::UintegerValue(3),
                                       ns3::MakeUintegerChecker<uint8_t>());
static ns3::GlobalValue g_reportTablePeriodicity("reportTablePeriodicity",
                                                 "Periodicity of RTs",
                                                 ns3::UintegerValue(1600),
                                                 ns3::MakeUintegerChecker<uint32_t>());
static ns3::GlobalValue g_outageThreshold("outageTh",
                                          "Outage threshold",
                                          ns3::DoubleValue(-5),
                                          ns3::MakeDoubleChecker<double>());
static ns3::GlobalValue g_lteUplink("lteUplink",
                                    "If true, always use LTE for uplink signalling",
                                    ns3::BooleanValue(false),
                                    ns3::MakeBooleanChecker());

int
main(int argc, char* argv[])
{
    LogComponentEnable("CfExample", LOG_DEBUG);
    LogComponentEnable("CfUnit", LOG_DEBUG);
    LogComponentEnable("CfUnit", LOG_FUNCTION);
    LogComponentEnable("VrServer", LOG_DEBUG);

    bool verbose = true;

    // The maximum X coordinate of the scenario
    double maxXAxis = 2800;
    // The maximum Y coordinate of the scenario
    double maxYAxis = 2800;
    // interside distance
    double isd = 850;

    uint8_t nGnb = 5;
    uint8_t ues = 1;
    std::string ueDist = "dr";

    CommandLine cmd(__FILE__);
    cmd.AddValue("verbose", "Tell application to log if true", verbose);
    cmd.AddValue("nGnb", "The number of gnb", nGnb);
    cmd.AddValue("ues", "The number of ue per gNB", ues);
    cmd.AddValue("ueDist", "Distribution of user locations.", ueDist);

    cmd.Parse(argc, argv);

    // A lengthy configuration
    bool harqEnabled = true;
    bool fixedTti = false;

    UintegerValue uintegerValue;
    BooleanValue booleanValue;
    StringValue stringValue;
    DoubleValue doubleValue;
    // EnumValue enumValue;
    // GlobalValue::GetValueByName("maxXAxis", doubleValue);
    // double maxXAxis = doubleValue.Get();
    // GlobalValue::GetValueByName("maxYAxis", doubleValue);
    // double maxYAxis = doubleValue.Get();

    // Variables for the RT
    int windowForTransient = 150; // number of samples for the vector to use in the filter
    GlobalValue::GetValueByName("reportTablePeriodicity", uintegerValue);
    int ReportTablePeriodicity = (int)uintegerValue.Get(); // in microseconds
    if (ReportTablePeriodicity == 1600)
    {
        windowForTransient = 150;
    }
    else if (ReportTablePeriodicity == 25600)
    {
        windowForTransient = 50;
    }
    else if (ReportTablePeriodicity == 12800)
    {
        windowForTransient = 100;
    }
    else
    {
        NS_ASSERT_MSG(false, "Unrecognized");
    }

    int vectorTransient = windowForTransient * ReportTablePeriodicity;

    // params for RT, filter, HO mode
    GlobalValue::GetValueByName("noiseAndFilter", booleanValue);
    bool noiseAndFilter = booleanValue.Get();
    GlobalValue::GetValueByName("handoverMode", uintegerValue);
    uint8_t hoMode = uintegerValue.Get();
    GlobalValue::GetValueByName("outageTh", doubleValue);
    double outageTh = doubleValue.Get();

    GlobalValue::GetValueByName("rlcAmEnabled", booleanValue);
    bool rlcAmEnabled = booleanValue.Get();
    GlobalValue::GetValueByName("bufferSize", uintegerValue);
    uint32_t bufferSize = uintegerValue.Get();
    // GlobalValue::GetValueByName("interPckInterval", uintegerValue);
    // uint32_t interPacketInterval = uintegerValue.Get();
    GlobalValue::GetValueByName("x2Latency", doubleValue);
    double x2Latency = doubleValue.Get();
    GlobalValue::GetValueByName("mmeLatency", doubleValue);
    double mmeLatency = doubleValue.Get();
    GlobalValue::GetValueByName("mobileSpeed", doubleValue);
    double ueSpeed = doubleValue.Get();

    double transientDuration = double(vectorTransient) / 1000000;

    NS_LOG_UNCOND("rlcAmEnabled " << rlcAmEnabled << " bufferSize " << bufferSize << " x2Latency "
                                  << x2Latency << " mmeLatency " << mmeLatency << " mobileSpeed "
                                  << ueSpeed);

    GlobalValue::GetValueByName("outPath", stringValue);
    std::string path = stringValue.Get();
    std::string mmWaveOutName = "MmWaveSwitchStats";
    std::string lteOutName = "LteSwitchStats";
    std::string dlRlcOutName = "DlRlcStats";
    std::string dlPdcpOutName = "DlPdcpStats";
    std::string ulRlcOutName = "UlRlcStats";
    std::string ulPdcpOutName = "UlPdcpStats";
    std::string ueHandoverStartOutName = "UeHandoverStartStats";
    std::string enbHandoverStartOutName = "EnbHandoverStartStats";
    std::string ueHandoverEndOutName = "UeHandoverEndStats";
    std::string enbHandoverEndOutName = "EnbHandoverEndStats";
    std::string cellIdInTimeOutName = "CellIdStats";
    std::string cellIdInTimeHandoverOutName = "CellIdStatsHandover";
    std::string mmWaveSinrOutputFilename = "MmWaveSinrTime";
    std::string x2statOutputFilename = "X2Stats";
    std::string udpSentFilename = "UdpSent";
    std::string udpReceivedFilename = "UdpReceived";
    std::string extension = ".txt";
    std::string version;
    version = "mc";
    Config::SetDefault("ns3::MmWaveUeMac::UpdateUeSinrEstimatePeriod", DoubleValue(0));

    // get current time
    time_t rawtime;
    struct tm* timeinfo;
    char buffer[80];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, 80, "%d_%m_%Y_%I_%M_%S", timeinfo);
    std::string time_str(buffer);

    Config::SetDefault("ns3::MmWaveHelper::RlcAmEnabled", BooleanValue(rlcAmEnabled));
    Config::SetDefault("ns3::MmWaveHelper::HarqEnabled", BooleanValue(harqEnabled));
    Config::SetDefault("ns3::MmWaveFlexTtiMacScheduler::HarqEnabled", BooleanValue(harqEnabled));
    Config::SetDefault("ns3::MmWaveFlexTtiMaxWeightMacScheduler::HarqEnabled",
                       BooleanValue(harqEnabled));
    Config::SetDefault("ns3::MmWaveFlexTtiMaxWeightMacScheduler::FixedTti", BooleanValue(fixedTti));
    Config::SetDefault("ns3::MmWaveFlexTtiMaxWeightMacScheduler::SymPerSlot", UintegerValue(6));
    Config::SetDefault("ns3::MmWavePhyMacCommon::TbDecodeLatency", UintegerValue(200.0));
    Config::SetDefault("ns3::MmWavePhyMacCommon::NumHarqProcess", UintegerValue(100));
    Config::SetDefault("ns3::ThreeGppChannelModel::UpdatePeriod", TimeValue(MilliSeconds(100.0)));
    Config::SetDefault("ns3::LteEnbRrc::SystemInformationPeriodicity",
                       TimeValue(MilliSeconds(5.0)));
    Config::SetDefault("ns3::LteRlcAm::ReportBufferStatusTimer", TimeValue(MicroSeconds(100.0)));
    Config::SetDefault("ns3::LteRlcUmLowLat::ReportBufferStatusTimer",
                       TimeValue(MicroSeconds(100.0)));
    Config::SetDefault("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue(320));
    Config::SetDefault("ns3::LteEnbRrc::FirstSibTime", UintegerValue(2));
    Config::SetDefault("ns3::MmWavePointToPointEpcHelper::X2LinkDelay",
                       TimeValue(MicroSeconds(x2Latency)));
    Config::SetDefault("ns3::MmWavePointToPointEpcHelper::X2LinkDataRate",
                       DataRateValue(DataRate("1000Gb/s")));
    Config::SetDefault("ns3::MmWavePointToPointEpcHelper::X2LinkMtu", UintegerValue(10000));
    Config::SetDefault("ns3::MmWavePointToPointEpcHelper::S1uLinkDelay",
                       TimeValue(MicroSeconds(1000)));
    Config::SetDefault("ns3::MmWavePointToPointEpcHelper::S1apLinkDelay",
                       TimeValue(MicroSeconds(mmeLatency)));
    Config::SetDefault("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue(bufferSize * 1024 * 1024));
    Config::SetDefault("ns3::LteRlcUmLowLat::MaxTxBufferSize",
                       UintegerValue(bufferSize * 1024 * 1024));
    Config::SetDefault("ns3::LteRlcAm::StatusProhibitTimer", TimeValue(MilliSeconds(10.0)));
    Config::SetDefault("ns3::LteRlcAm::MaxTxBufferSize", UintegerValue(bufferSize * 1024 * 1024));

    // handover and RT related params
    switch (hoMode)
    {
    case 1:
        Config::SetDefault("ns3::LteEnbRrc::SecondaryCellHandoverMode",
                           EnumValue(LteEnbRrc::THRESHOLD));
        break;
    case 2:
        Config::SetDefault("ns3::LteEnbRrc::SecondaryCellHandoverMode",
                           EnumValue(LteEnbRrc::FIXED_TTT));
        break;
    case 3:
        Config::SetDefault("ns3::LteEnbRrc::SecondaryCellHandoverMode",
                           EnumValue(LteEnbRrc::DYNAMIC_TTT));
        break;
    }

    Config::SetDefault("ns3::LteEnbRrc::FixedTttValue", UintegerValue(150));
    Config::SetDefault("ns3::LteEnbRrc::CrtPeriod", IntegerValue(ReportTablePeriodicity));
    Config::SetDefault("ns3::LteEnbRrc::OutageThreshold", DoubleValue(outageTh));
    Config::SetDefault("ns3::MmWaveEnbPhy::UpdateSinrEstimatePeriod",
                       IntegerValue(ReportTablePeriodicity));
    Config::SetDefault("ns3::MmWaveEnbPhy::Transient", IntegerValue(vectorTransient));
    Config::SetDefault("ns3::MmWaveEnbPhy::NoiseAndFilter", BooleanValue(noiseAndFilter));

    // set the type of RRC to use, i.e., ideal or real
    // by setting the following two attributes to true, the simulation will use
    // the ideal paradigm, meaning no packets are sent. in fact, only the callbacks are triggered
    Config::SetDefault("ns3::MmWaveHelper::UseIdealRrc", BooleanValue(true));

    GlobalValue::GetValueByName("lteUplink", booleanValue);
    bool lteUplink = booleanValue.Get();

    Config::SetDefault("ns3::McUePdcp::LteUplink", BooleanValue(lteUplink));
    std::cout << "Lte uplink " << lteUplink << "\n";

    // settings for the 3GPP the channel
    Config::SetDefault("ns3::ThreeGppChannelModel::UpdatePeriod",
                       TimeValue(MilliSeconds(
                           100))); // interval after which the channel for a moving user is updated,
    // Config::SetDefault("ns3::ThreeGppChannelModel::Blockage",
    //                    BooleanValue(true)); // use blockage or not
    // Config::SetDefault("ns3::ThreeGppChannelModel::PortraitMode",
    //                    BooleanValue(true)); // use blockage model with UT in portrait mode
    // Config::SetDefault("ns3::ThreeGppChannelModel::NumNonselfBlocking",
    //                    IntegerValue(4)); // number of non-self blocking obstacles

    // by default, isotropic antennas are used. To use the 3GPP radiation pattern instead, use the
    // <ThreeGppAntennaArrayModel> beware: proper configuration of the bearing and downtilt angles
    // is needed
    Config::SetDefault("ns3::PhasedArrayModel::AntennaElement",
                       PointerValue(CreateObject<IsotropicAntennaModel>()));

    Ptr<MmWaveHelper> mmwaveHelper = CreateObject<MmWaveHelper>();
    mmwaveHelper->SetPathlossModelType("ns3::ThreeGppUmiStreetCanyonPropagationLossModel");
    // mmwaveHelper->SetChannelConditionModelType("ns3::BuildingsChannelConditionModel");

    // set the number of antennas for both UEs and eNBs
    mmwaveHelper->SetUePhasedArrayModelAttribute("NumColumns", UintegerValue(4));
    mmwaveHelper->SetUePhasedArrayModelAttribute("NumRows", UintegerValue(4));
    mmwaveHelper->SetEnbPhasedArrayModelAttribute("NumColumns", UintegerValue(8));
    mmwaveHelper->SetEnbPhasedArrayModelAttribute("NumRows", UintegerValue(8));

    Ptr<MmWavePointToPointEpcHelper> epcHelper = CreateObject<MmWavePointToPointEpcHelper>();
    mmwaveHelper->SetEpcHelper(epcHelper);
    mmwaveHelper->SetHarqEnabled(harqEnabled);
    mmwaveHelper->Initialize();

    ConfigStore inputConfig;
    inputConfig.ConfigureDefaults();

    // Get SGW/PGW and create a single RemoteHost
    Ptr<Node> pgw = epcHelper->GetPgwNode();
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer);

    // Create the Internet by connecting remoteHost to pgw. Setup routing too
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
    p2ph.SetDeviceAttribute("Mtu", UintegerValue(2500));
    p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
    // interface 0 is localhost, 1 is the p2p device
    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress(1);
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
        ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);

    // create LTE, mmWave eNB nodes and UE node
    NodeContainer ueNodes;
    NodeContainer mmWaveEnbNodes;
    NodeContainer lteEnbNodes;
    NodeContainer allEnbNodes;
    mmWaveEnbNodes.Create(2);
    lteEnbNodes.Create(1);
    ueNodes.Create(1);
    allEnbNodes.Add(lteEnbNodes);
    allEnbNodes.Add(mmWaveEnbNodes);

    // 为用户和基站分配移动性模型
    Vector centerPosition = Vector(maxXAxis / 2, maxYAxis / 2, BS_HEIGHT);
    Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator>();
    Ptr<ListPositionAllocator> gnbPositionAlloc = CreateObject<ListPositionAllocator>();
    Ptr<ListPositionAllocator> uePositionAlloc = CreateObject<ListPositionAllocator>();

    // 起控制作用的 enb 位于地图中央
    enbPositionAlloc->Add(centerPosition);
    MobilityHelper enbmobility;
    enbmobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    enbmobility.SetPositionAllocator(enbPositionAlloc);
    enbmobility.Install(lteEnbNodes);

    // 在中央部署一个 gnb，其余环绕一周
    double x, y;
    double nConstellation = nGnb;
    Ptr<UniformRandomVariable> distR = CreateObject<UniformRandomVariable>();
    Ptr<UniformRandomVariable> distTheta = CreateObject<UniformRandomVariable>();

    distR->SetAttribute("Min", DoubleValue(MIN_BS_UT_DISTANCE));
    distR->SetAttribute("Max", DoubleValue(isd / 2));
    distTheta->SetAttribute("Min", DoubleValue(-1.0 * M_PI));
    distTheta->SetAttribute("Max", DoubleValue(M_PI));

    Ptr<UniformRandomVariable> ueX = CreateObject<UniformRandomVariable>();
    Ptr<UniformRandomVariable> ueY = CreateObject<UniformRandomVariable>();
    ueX->SetAttribute("Min", DoubleValue(maxXAxis));
    ueY->SetAttribute("Max", DoubleValue(maxYAxis));

    for (int8_t i = 0; i < nConstellation; ++i)
    {
        if (i == 0)
        {
            gnbPositionAlloc->Add(centerPosition);
            x = 0;
            y = 0;
        }
        else
        {
            x = isd * cos((2 * M_PI * (i - 1)) / (nConstellation - 1) + M_PI / 4);
            y = isd * sin((2 * M_PI * (i - 1)) / (nConstellation - 1) + M_PI / 4);
            gnbPositionAlloc->Add(Vector(centerPosition.x + x, centerPosition.y + y, BS_HEIGHT));
        }

        for (uint32_t j = 0; j < ues; j++)
        {
            if (ueDist == "dr")
            {
                double rho = distR->GetValue();
                double theta = distTheta->GetValue();
                double utX = centerPosition.x + x + rho * cos(theta);
                double utY = centerPosition.y + y + rho * sin(theta);
                uePositionAlloc->Add(Vector(utX, utY, UT_HEIGHT));
            }
            else if (ueDist == "un")
            {
                uePositionAlloc->Add(Vector(ueX->GetValue(), ueY->GetValue(), UT_HEIGHT));
            }
            // NS_LOG_DEBUG("x " << utX << " y " << utY);
        }
    }

    MobilityHelper gnbmobility;
    gnbmobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    gnbmobility.SetPositionAllocator(gnbPositionAlloc);
    gnbmobility.Install(mmWaveEnbNodes);

    MobilityHelper uemobility;
    Ptr<UniformRandomVariable> speed = CreateObject<UniformRandomVariable>();
    speed->SetAttribute("Min", DoubleValue(2.0));
    speed->SetAttribute("Max", DoubleValue(4.0));

    uemobility.SetMobilityModel("ns3::RandomWalk2dOutdoorMobilityModel",
                                "Speed",
                                PointerValue(speed),
                                "Bounds",
                                RectangleValue(Rectangle(0, maxXAxis, 0, maxYAxis)));
    uemobility.SetPositionAllocator(uePositionAlloc);
    uemobility.Install(ueNodes);

    // Install mmWave, lte, mc Devices to the nodes
    NetDeviceContainer lteEnbDevs = mmwaveHelper->InstallLteEnbDevice(lteEnbNodes);
    NetDeviceContainer mmWaveEnbDevs = mmwaveHelper->InstallEnbDevice(mmWaveEnbNodes);
    NetDeviceContainer mcUeDevs;
    mcUeDevs = mmwaveHelper->InstallMcUeDevice(ueNodes);

    // Install the IP stack on the UEs
    internet.Install(ueNodes);
    Ipv4InterfaceContainer ueIpIface;
    ueIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(mcUeDevs));
    // Assign IP address to UEs, and install applications
    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        Ptr<Node> ueNode = ueNodes.Get(u);
        // Set the default gateway for the UE
        Ptr<Ipv4StaticRouting> ueStaticRouting =
            ipv4RoutingHelper.GetStaticRouting(ueNode->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
    }

    // Add X2 interfaces
    mmwaveHelper->AddX2Interface(lteEnbNodes, mmWaveEnbNodes);

    // Manual attachment
    mmwaveHelper->AttachToClosestEnb(mcUeDevs, mmWaveEnbDevs, lteEnbDevs);

    ObjectFactory cfUnitObj;
    cfUnitObj.SetTypeId("ns3::CfUnit");
    CfModel cfModel("GPU", 82.6);
    cfUnitObj.Set("EnableAutoSchedule", BooleanValue(false));
    cfUnitObj.Set("CfModel", CfModelValue(cfModel));

    Ptr<CfRanHelper> cfRanHelper = CreateObject<CfRanHelper>();
    cfRanHelper->InstallCfUnit(mmWaveEnbNodes, cfUnitObj);

    Ptr<CfranSystemInfo> cfranSystemInfo = CreateObject<CfranSystemInfo>();

    for (uint32_t u = 0; u < mcUeDevs.GetN(); ++u)
    {
        Ptr<McUeNetDevice> mcUeDev = DynamicCast<McUeNetDevice>(mcUeDevs.Get(u));
        uint64_t imsi = mcUeDev->GetImsi();

        CfranSystemInfo::UeInfo ueInfo;
        UeTaskModel ueTaskModel;
        // ueTaskModel.m_cfRequired = CfModel("GPU", 10);
        ueTaskModel.m_cfLoad = 0.2;
        ueTaskModel.m_deadline = 10;

        ueInfo.m_imsi = imsi;
        ueInfo.m_taskModel = ueTaskModel;
        ueInfo.m_taskPeriodity = 16;
        ueInfo.m_mcUeNetDevice = mcUeDev;

        cfranSystemInfo->AddUeInfo(imsi, ueInfo);
    }

    Config::SetDefault("ns3::VrServer::CfranSystemInfo", PointerValue(cfranSystemInfo));

    Ptr<VrServerHelper> vrServerHelper = Create<VrServerHelper>();
    ApplicationContainer apps = vrServerHelper->Install(mmWaveEnbNodes);
    Ptr<CfApplication> vrApp = DynamicCast<CfApplication>(apps.Get(0));
    DynamicCast<VrServer>(vrApp)->StartServiceForImsi(1);

    PrintGnuplottableUeListToFile("ues.txt");
    PrintGnuplottableEnbListToFile("enbs.txt");
    /* ... */
    // CfUnit cfUnitExample();
    // Config::SetDefault("ns3::CfUnit::EnableAutoSchedule", BooleanValue(false));
    // Ptr<CfUnit> cfUnitExample = CreateObject<CfUnit>();

    // cfUnitExample->SetAttribute("EnableAutoSchedule", BooleanValue(false));

    // CfModel cfRequired("GPU", 10);
    // UeTaskModel ueTask(1, cfRequired, 20, 10);
    Simulator::Schedule(Seconds(1), &TargetEnbTest);

    Simulator::Stop(Seconds(10));
    // cfUnitExample->AddNewUeTaskForSchedule(1, ueTask);
    Simulator::Run();

    Simulator::Destroy();
    return 0;
}
