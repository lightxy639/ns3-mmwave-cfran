/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/* *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Michele Polese <michele.polese@gmail.com>
 */

#include "ns3/applications-module.h"
#include "ns3/buildings-helper.h"
#include "ns3/buildings-module.h"
#include "ns3/command-line.h"
#include "ns3/config-store-module.h"
#include "ns3/epc-helper.h"
#include "ns3/global-value.h"
#include "ns3/internet-module.h"
#include "ns3/isotropic-antenna-model.h"
#include "ns3/mmwave-helper.h"
#include "ns3/mmwave-point-to-point-epc-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/node-list.h"
#include "ns3/point-to-point-helper.h"
#include <ns3/cf-application-helper.h>
#include <ns3/cfran-helper.h>
#include <ns3/lte-ue-net-device.h>
#include <ns3/random-variable-stream.h>
#include <ns3/ue-cf-application-helper.h>

#include <ctime>
#include <iostream>
#include <list>
#include <stdlib.h>

using namespace ns3;
using namespace mmwave;

/**
 * Sample simulation script for MC devices.
 * One LTE and two MmWave eNodeBs are instantiated, and a MC UE device is placed between them.
 * In particular, the UE is initially closer to one of the two BSs, and progressively moves towards
 * the other. During the course of the simulation multiple handovers occur, due to the changing
 * distance between the devices and the presence of obstacles, which can obstruct the LOS path
 * between the UE and the eNBs.
 */

NS_LOG_COMPONENT_DEFINE("McTwoEnbs");

void
PrintGnuplottableBuildingListToFile(std::string filename)
{
    std::ofstream outFile;
    outFile.open(filename.c_str(), std::ios_base::out | std::ios_base::trunc);
    if (!outFile.is_open())
    {
        NS_LOG_ERROR("Can't open file " << filename);
        return;
    }
    uint32_t index = 0;
    for (BuildingList::Iterator it = BuildingList::Begin(); it != BuildingList::End(); ++it)
    {
        ++index;
        Box box = (*it)->GetBoundaries();
        outFile << "set object " << index << " rect from " << box.xMin << "," << box.yMin << " to "
                << box.xMax << "," << box.yMax << " front fs empty " << std::endl;
    }
}

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
ChangePosition(Ptr<Node> node, Vector vector)
{
    Ptr<MobilityModel> model = node->GetObject<MobilityModel>();
    model->SetPosition(vector);
    NS_LOG_UNCOND("************************--------------------Change "
                  "Position-------------------------------*****************");
}

void
ChangeSpeed(Ptr<Node> n, Vector speed)
{
    n->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(speed);
    NS_LOG_UNCOND("************************--------------------Change "
                  "Speed-------------------------------*****************");
}

void
PrintPosition(Ptr<Node> node)
{
    Ptr<MobilityModel> model = node->GetObject<MobilityModel>();
    NS_LOG_UNCOND("Position +****************************** " << model->GetPosition() << " at time "
                                                              << Simulator::Now().GetSeconds());
}

void
PrintLostUdpPackets(Ptr<UdpServer> app, std::string fileName)
{
    std::ofstream logFile(fileName.c_str(), std::ofstream::app);
    logFile << Simulator::Now().GetSeconds() << " " << app->GetLost() << std::endl;
    logFile.close();
    Simulator::Schedule(MilliSeconds(20), &PrintLostUdpPackets, app, fileName);
}

bool
AreOverlapping(Box a, Box b)
{
    return !((a.xMin > b.xMax) || (b.xMin > a.xMax) || (a.yMin > b.yMax) || (b.yMin > a.yMax));
}

bool
OverlapWithAnyPrevious(Box box, std::list<Box> m_previousBlocks)
{
    for (std::list<Box>::iterator it = m_previousBlocks.begin(); it != m_previousBlocks.end(); ++it)
    {
        if (AreOverlapping(*it, box))
        {
            return true;
        }
    }
    return false;
}

std::pair<Box, std::list<Box>>
GenerateBuildingBounds(double xArea,
                       double yArea,
                       double maxBuildSize,
                       std::list<Box> m_previousBlocks)
{
    Ptr<UniformRandomVariable> xMinBuilding = CreateObject<UniformRandomVariable>();
    xMinBuilding->SetAttribute("Min", DoubleValue(30));
    xMinBuilding->SetAttribute("Max", DoubleValue(xArea));

    NS_LOG_UNCOND("min " << 0 << " max " << xArea);

    Ptr<UniformRandomVariable> yMinBuilding = CreateObject<UniformRandomVariable>();
    yMinBuilding->SetAttribute("Min", DoubleValue(0));
    yMinBuilding->SetAttribute("Max", DoubleValue(yArea));

    NS_LOG_UNCOND("min " << 0 << " max " << yArea);

    Box box;
    uint32_t attempt = 0;
    do
    {
        NS_ASSERT_MSG(attempt < 100,
                      "Too many failed attempts to position non-overlapping buildings. Maybe area "
                      "too small or too many buildings?");
        box.xMin = xMinBuilding->GetValue();

        Ptr<UniformRandomVariable> xMaxBuilding = CreateObject<UniformRandomVariable>();
        xMaxBuilding->SetAttribute("Min", DoubleValue(box.xMin));
        xMaxBuilding->SetAttribute("Max", DoubleValue(box.xMin + maxBuildSize));
        box.xMax = xMaxBuilding->GetValue();

        box.yMin = yMinBuilding->GetValue();

        Ptr<UniformRandomVariable> yMaxBuilding = CreateObject<UniformRandomVariable>();
        yMaxBuilding->SetAttribute("Min", DoubleValue(box.yMin));
        yMaxBuilding->SetAttribute("Max", DoubleValue(box.yMin + maxBuildSize));
        box.yMax = yMaxBuilding->GetValue();

        ++attempt;
    } while (OverlapWithAnyPrevious(box, m_previousBlocks));

    NS_LOG_UNCOND("Building in coordinates (" << box.xMin << " , " << box.yMin << ") and ("
                                              << box.xMax << " , " << box.yMax
                                              << ") accepted after " << attempt << " attempts");
    m_previousBlocks.push_back(box);
    std::pair<Box, std::list<Box>> pairReturn = std::make_pair(box, m_previousBlocks);
    return pairReturn;
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

void
PacketSinkLog(Ptr<const Packet>, const Address& from, const Address& local)
{
    NS_LOG_UNCOND("Recv packet "
                  << "from " << InetSocketAddress::ConvertFrom(from).GetIpv4() << " at "
                  << InetSocketAddress::ConvertFrom(local).GetIpv4());
}

static ns3::GlobalValue g_mmw1DistFromMainStreet(
    "mmw1Dist",
    "Distance from the main street of the first MmWaveEnb",
    ns3::UintegerValue(50),
    ns3::MakeUintegerChecker<uint32_t>());
static ns3::GlobalValue g_mmw2DistFromMainStreet(
    "mmw2Dist",
    "Distance from the main street of the second MmWaveEnb",
    ns3::UintegerValue(50),
    ns3::MakeUintegerChecker<uint32_t>());
static ns3::GlobalValue g_mmw3DistFromMainStreet(
    "mmw3Dist",
    "Distance from the main street of the third MmWaveEnb",
    ns3::UintegerValue(110),
    ns3::MakeUintegerChecker<uint32_t>());
static ns3::GlobalValue g_mmWaveDistance("mmWaveDist",
                                         "Distance between MmWave eNB 1 and 2",
                                         ns3::UintegerValue(200),
                                         ns3::MakeUintegerChecker<uint32_t>());
static ns3::GlobalValue g_numBuildingsBetweenMmWaveEnb(
    "numBlocks",
    "Number of buildings between MmWave eNB 1 and 2",
    ns3::UintegerValue(0),
    ns3::MakeUintegerChecker<uint32_t>());
static ns3::GlobalValue g_interPckInterval("interPckInterval",
                                           "Interarrival time of UDP packets (us)",
                                           ns3::UintegerValue(20),
                                           ns3::MakeUintegerChecker<uint32_t>());
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
static ns3::GlobalValue g_maxXAxis(
    "maxXAxis",
    "The maximum X coordinate for the area in which to deploy the buildings",
    ns3::DoubleValue(150),
    ns3::MakeDoubleChecker<double>());
static ns3::GlobalValue g_maxYAxis(
    "maxYAxis",
    "The maximum Y coordinate for the area in which to deploy the buildings",
    ns3::DoubleValue(40),
    ns3::MakeDoubleChecker<double>());
static ns3::GlobalValue g_outPath("outPath",
                                  "The path of output log files",
                                  ns3::StringValue("./mmwave-mc-out"),
                                  ns3::MakeStringChecker());
static ns3::GlobalValue g_noiseAndFilter(
    "noiseAndFilter",
    "If true, use noisy SINR samples, filtered. If false, just use the SINR measure",
    ns3::BooleanValue(false),
    ns3::MakeBooleanChecker());
static ns3::GlobalValue g_handoverMode("handoverMode",
                                       "Handover mode",
                                       ns3::UintegerValue(4),
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
    // LogComponentEnable("McTwoEnbs", LOG_DEBUG);
    LogComponentEnable("CfApplication", LOG_INFO);
    LogComponentEnable("CfApplication", LOG_PREFIX_ALL);
    // LogComponentEnable("CfApplicationHelper", LOG_FUNCTION);
    LogComponentEnable("UeCfApplication", LOG_INFO);
    // LogComponentEnable("UeCfApplication", LOG_FUNCTION);
    LogComponentEnable("UeCfApplication", LOG_PREFIX_ALL);

    LogComponentEnable("LteEnbRrc", LOG_INFO);
    LogComponentEnable("LteEnbRrc", LOG_DEBUG);
    LogComponentEnable("LteEnbRrc", LOG_PREFIX_ALL);
    // LogComponentEnable("MultiPacketManager", LOG_LEVEL_FUNCTION);
    // LogComponentEnable("MultiPacketManager", LOG_LEVEL_DEBUG);
    // LogComponentEnable("MultiPacketManager", LOG_PREFIX_ALL);
    // LogComponentEnable("CfUnitUeIso", LOG_FUNCTION);
    // LogComponentEnable("CfUnitUeIso", LOG_FUNCTION);
    // LogComponentEnable("McUeNetDevice", LOG_LOGIC);
    // LogComponentEnable("MmWaveNetDevice", LOG_LOGIC);
    // LogComponentEnable("LteUeRrc", LOG_FUNCTION);
    // LogComponentEnable("LteUeRrc", LOG_INFO);
    // LogComponentEnable("LteEnbRrc", LOG_INFO);
    // LogComponentEnable("LteEnbRrc", LOG_FUNCTION);
    // LogComponentEnable("LteRlcAm", LOG_FUNCTION);
    // LogComponentEnable("LteNetDevice", LOG_FUNCTION);
    // LogComponentEnable("LteNetDevice", LOG_DEBUG);
    // LogComponentEnable("LteRlcAm", LOG_DEBUG);
    // LogComponentEnable("MmWaveNetDevice", LOG_LOGIC);
    // LogComponentEnable("MmWavePointToPointEpcHelper", LOG_LOGIC);
    // LogComponentEnable("LteRlcAm", LOG_INFO);
    LogComponentEnable("McTwoEnbs", LOG_DEBUG);

    bool harqEnabled = true;
    bool fixedTti = false;

    std::list<Box> m_previousBlocks;

    // Command line arguments
    CommandLine cmd;
    cmd.Parse(argc, argv);

    UintegerValue uintegerValue;
    BooleanValue booleanValue;
    StringValue stringValue;
    DoubleValue doubleValue;
    // EnumValue enumValue;
    GlobalValue::GetValueByName("numBlocks", uintegerValue);
    uint32_t numBlocks = uintegerValue.Get();
    GlobalValue::GetValueByName("maxXAxis", doubleValue);
    double maxXAxis = doubleValue.Get();
    GlobalValue::GetValueByName("maxYAxis", doubleValue);
    double maxYAxis = doubleValue.Get();

    double ueInitialPosition = 90;
    double ueFinalPosition = 110;

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
    GlobalValue::GetValueByName("interPckInterval", uintegerValue);
    uint32_t interPacketInterval = uintegerValue.Get();
    GlobalValue::GetValueByName("x2Latency", doubleValue);
    double x2Latency = doubleValue.Get();
    GlobalValue::GetValueByName("mmeLatency", doubleValue);
    double mmeLatency = doubleValue.Get();
    GlobalValue::GetValueByName("mobileSpeed", doubleValue);
    double ueSpeed = doubleValue.Get();

    double transientDuration = double(vectorTransient) / 1000000;
    double simTime =
        transientDuration + ((double)ueFinalPosition - (double)ueInitialPosition) / ueSpeed + 1;

    NS_LOG_UNCOND("rlcAmEnabled " << rlcAmEnabled << " bufferSize " << bufferSize
                                  << " interPacketInterval " << interPacketInterval << " x2Latency "
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

    // Config::SetDefault("ns3::PacketSink::RxWithAddresses", MakeCallback(&PacketSinkLog));
    // Config::Connect("ns3::PacketSink::RxWithAddresses", MakeCallback(&PacketSinkLog));

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
    case 4:
        Config::SetDefault("ns3::LteEnbRrc::SecondaryCellHandoverMode",
                            EnumValue(LteEnbRrc::NO_AUTO));
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
    Config::SetDefault("ns3::ThreeGppChannelModel::Blockage",
                       BooleanValue(true)); // use blockage or not
    Config::SetDefault("ns3::ThreeGppChannelModel::PortraitMode",
                       BooleanValue(true)); // use blockage model with UT in portrait mode
    Config::SetDefault("ns3::ThreeGppChannelModel::NumNonselfBlocking",
                       IntegerValue(4)); // number of non-self blocking obstacles

    // by default, isotropic antennas are used. To use the 3GPP radiation pattern instead, use the
    // <ThreeGppAntennaArrayModel> beware: proper configuration of the bearing and downtilt angles
    // is needed
    Config::SetDefault("ns3::PhasedArrayModel::AntennaElement",
                       PointerValue(CreateObject<IsotropicAntennaModel>()));

    Ptr<MmWaveHelper> mmwaveHelper = CreateObject<MmWaveHelper>();
    mmwaveHelper->SetPathlossModelType("ns3::ThreeGppUmiStreetCanyonPropagationLossModel");
    mmwaveHelper->SetChannelConditionModelType("ns3::BuildingsChannelConditionModel");

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

    // parse again so you can override default values from the command line
    cmd.Parse(argc, argv);

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

    // Positions
    Vector mmw1Position = Vector(50, 70, 3);
    Vector mmw2Position = Vector(150, 70, 3);

    std::vector<Ptr<Building>> buildingVector;

    double maxBuildingSize = 20;

    for (uint32_t buildingIndex = 0; buildingIndex < numBlocks; buildingIndex++)
    {
        Ptr<Building> building;
        building = Create<Building>();
        /* returns a vecotr where:
         * position [0]: coordinates for x min
         * position [1]: coordinates for x max
         * position [2]: coordinates for y min
         * position [3]: coordinates for y max
         */
        std::pair<Box, std::list<Box>> pairBuildings =
            GenerateBuildingBounds(maxXAxis, maxYAxis, maxBuildingSize, m_previousBlocks);
        m_previousBlocks = std::get<1>(pairBuildings);
        Box box = std::get<0>(pairBuildings);
        Ptr<UniformRandomVariable> randomBuildingZ = CreateObject<UniformRandomVariable>();
        randomBuildingZ->SetAttribute("Min", DoubleValue(1.6));
        randomBuildingZ->SetAttribute("Max", DoubleValue(40));
        double buildingHeight = randomBuildingZ->GetValue();

        building->SetBoundaries(Box(box.xMin, box.xMax, box.yMin, box.yMax, 0.0, buildingHeight));
        buildingVector.push_back(building);
    }

    // Install Mobility Model
    Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator>();
    // enbPositionAlloc->Add (Vector ((double)mmWaveDist/2 + streetWidth, mmw1Dist + 2*streetWidth,
    // mmWaveZ));
    enbPositionAlloc->Add(mmw1Position); // LTE BS, out of area where buildings are deployed
    enbPositionAlloc->Add(mmw1Position);
    enbPositionAlloc->Add(mmw2Position);
    MobilityHelper enbmobility;
    enbmobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    enbmobility.SetPositionAllocator(enbPositionAlloc);
    enbmobility.Install(allEnbNodes);
    BuildingsHelper::Install(allEnbNodes);

    MobilityHelper uemobility;
    Ptr<ListPositionAllocator> uePositionAlloc = CreateObject<ListPositionAllocator>();
    // uePositionAlloc->Add (Vector (ueInitialPosition, -5, 0));
    uePositionAlloc->Add(Vector(ueInitialPosition, -5, 1.6));
    uemobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    uemobility.SetPositionAllocator(uePositionAlloc);
    uemobility.Install(ueNodes);
    BuildingsHelper::Install(ueNodes);

    ueNodes.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(60, 70, 1.6));
    ueNodes.Get(0)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0, 0, 0));

    // ueNodes.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (ueInitialPosition,
    // -5, 1.6)); ueNodes.Get(0)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0,
    // 0, 0)); ueNodes.Get(1)->GetObject<MobilityModel>()->SetPosition(
    //     Vector(60, 70, 1.6));
    // ueNodes.Get(1)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0, 0, 0));
    // Install mmWave, lte, mc Devices to the nodes
    NetDeviceContainer lteEnbDevs = mmwaveHelper->InstallLteEnbDevice(lteEnbNodes);
    NetDeviceContainer mmWaveEnbDevs = mmwaveHelper->InstallEnbDevice(mmWaveEnbNodes);
    NetDeviceContainer mcUeDevs;
    mcUeDevs = mmwaveHelper->InstallMcUeDevice(ueNodes);

    // Assign custom IP addresses to enb to facilitate the use of custom applications
    Ipv4AddressHelper enbIpv4h;
    enbIpv4h.SetBase("2.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer enbIpIfaces = enbIpv4h.Assign(mmWaveEnbDevs);

    Ptr<CfranSystemInfo> cfranSystemInfo = CreateObject<CfranSystemInfo>();

    for (uint32_t n = 0; n < mmWaveEnbDevs.GetN(); n++)
    {
        Ptr<NetDevice> tempMmNetDev = mmWaveEnbDevs.Get(n);
        Ptr<MmWaveEnbNetDevice> mmNetDev = DynamicCast<MmWaveEnbNetDevice>(tempMmNetDev);
        Ipv4Address gnbAddr = enbIpIfaces.GetAddress(n);

        CfranSystemInfo::CellInfo cellInfo;
        cellInfo.m_id = mmNetDev->GetCellId();
        cellInfo.m_mmwaveEnbNetDevice = mmNetDev;
        cellInfo.m_ipAddrToUe = gnbAddr;

        cfranSystemInfo->AddCellInfo(cellInfo.m_id, cellInfo);
        NS_LOG_DEBUG("Add CellInfo " << mmNetDev->GetCellId());
    }

    // interface 0 is localhost, 1 is the p2p device
    // Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress(1);
    for (uint32_t n = 0; n < mmWaveEnbNodes.GetN(); ++n)
    {
        Ptr<Node> enbNode = mmWaveEnbNodes.Get(n);
        Ptr<Ipv4> enbIpv4 = enbNode->GetObject<Ipv4>();

        Ptr<NetDevice> enbNetDev = mmWaveEnbDevs.Get(n);

        Ptr<Ipv4StaticRouting> enbStaticRouting = ipv4RoutingHelper.GetStaticRouting(enbIpv4);
        enbStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"),
                                            Ipv4Mask("255.0.0.0"),
                                            enbIpv4->GetInterfaceForDevice(enbNetDev));
        NS_LOG_DEBUG("Add Interface " << enbIpv4->GetInterfaceForDevice(enbNetDev));
    }

    // Install the IP stack on the UEs
    internet.Install(ueNodes);
    Ipv4InterfaceContainer ueIpIface;
    ueIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(mcUeDevs));
    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        Ipv4Address ueAddr = ueIpIface.GetAddress(u);
        NS_LOG_DEBUG("The ip addr of ue " << u << " is " << ueAddr);
    }
    // Assign IP address to UEs, and install applications
    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        Ptr<Node> ueNode = ueNodes.Get(u);
        // Set the default gateway for the UE
        Ptr<Ipv4StaticRouting> ueStaticRouting =
            ipv4RoutingHelper.GetStaticRouting(ueNode->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);

        ueStaticRouting->AddNetworkRouteTo(Ipv4Address("2.0.0.0"), Ipv4Mask("255.0.0.0"), 1);
    }

    // Add X2 interfaces
    mmwaveHelper->AddX2Interface(lteEnbNodes, mmWaveEnbNodes);

    // Manual attachment

    mmwaveHelper->AttachToClosestEnb(mcUeDevs, mmWaveEnbDevs, lteEnbDevs);

    ObjectFactory cfUnitObj;
    cfUnitObj.SetTypeId("ns3::CfUnitUeIso");
    CfModel cfModel("GPU", 82.6);
    // cfUnitObj.Set("EnableAutoSchedule", BooleanVaslue(false));
    cfUnitObj.Set("CfModel", CfModelValue(cfModel));

    Ptr<CfRanHelper> cfRanHelper = CreateObject<CfRanHelper>();
    cfRanHelper->InstallCfUnit(mmWaveEnbNodes, cfUnitObj);

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
    Config::SetDefault("ns3::CfApplication::CfranSystemInfomation", PointerValue(cfranSystemInfo));
    Config::SetDefault("ns3::UeCfApplication::CfranSystemInfomation",
                       PointerValue(cfranSystemInfo));

    // Install and start applications on UEs and remote host
    uint16_t dlPort = 1234;
    uint16_t ulPort = 2000;
    ApplicationContainer clientApps;
    ApplicationContainer serverApps;

    ApplicationContainer gnbApps;
    bool dl = 0;
    bool ul = 1;
    Config::SetDefault("ns3::CfApplication::Port", UintegerValue(ulPort));
    Config::SetDefault("ns3::UeCfApplication::OffloadPort", UintegerValue(ulPort));

    CfApplicationHelper cfAppHelper;
    serverApps.Add(cfAppHelper.Install(mmWaveEnbNodes));

    UeCfApplicationHelper ueCfAppHelper;
    clientApps.Add(ueCfAppHelper.Install(ueNodes));

    // Start applications
    NS_LOG_UNCOND("transientDuration " << transientDuration << " simTime " << simTime);
    serverApps.Start(Seconds(transientDuration));
    clientApps.Start(Seconds(transientDuration + 1));
    clientApps.Stop(Seconds(simTime - 1));

    Simulator::Schedule(Seconds(transientDuration),
                        &ChangeSpeed,
                        ueNodes.Get(0),
                        Vector(0, 0, 0)); // start UE movement after Seconds(0.5)
    // Vector(ueSpeed, 0, 0)); // start UE movement after Seconds(0.5)
    Simulator::Schedule(Seconds(simTime - 1),
                        &ChangeSpeed,
                        ueNodes.Get(0),
                        Vector(0, 0, 0)); // start UE movement after Seconds(0.5)
    // Hasty test
    // Ptr<CfApplication> cfApp = DynamicCast<CfApplication> (serverApps.Get(0));
    // Simulator::Schedule(Seconds(transientDuration + 2), &CfApplication::MigrateUeService, cfApp,
    // 1, 3);
    Simulator::Schedule(Seconds(1.5),
                        &LteEnbRrc::PerformHandoverToTargetCell,
                        DynamicCast<LteEnbNetDevice>(lteEnbDevs.Get(0))->GetRrc(),
                        1,
                        3);

    Simulator::Schedule(Seconds(2.5),
                        &LteEnbRrc::PerformHandoverToTargetCell,
                        DynamicCast<LteEnbNetDevice>(lteEnbDevs.Get(0))->GetRrc(),
                        1,
                        2);
                        
    double numPrints = 0;
    for (int i = 0; i < numPrints; i++)
    {
        Simulator::Schedule(Seconds(i * simTime / numPrints), &PrintPosition, ueNodes.Get(0));
    }

    // Simulator::Schedule(Seconds(1), &TargetEnbTest);

    mmwaveHelper->EnableTraces();

    // set to true if you want to print the map of buildings, ues and enbs
    bool print = false;
    if (print)
    {
        PrintGnuplottableBuildingListToFile("buildings.txt");
        PrintGnuplottableUeListToFile("ues.txt");
        PrintGnuplottableEnbListToFile("enbs.txt");
    }
    else
    {
        Simulator::Stop(Seconds(simTime));
        Simulator::Run();
    }

    Simulator::Destroy();
    return 0;
}
