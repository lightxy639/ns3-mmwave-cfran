/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 *   Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *   Copyright (c) 2015, NYU WIRELESS, Tandon School of Engineering, New York University
 *   Copyright (c) 2016, 2018, University of Padova, Dep. of Information Engineering, SIGNET lab.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2 as
 *   published by the Free Software Foundation;
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *   Author: Marco Miozzo <marco.miozzo@cttc.es>
 *           Nicola Baldo  <nbaldo@cttc.es>
 *
 *   Modified by: Marco Mezzavilla < mezzavilla@nyu.edu>
 *                         Sourjya Dutta <sdutta@nyu.edu>
 *                         Russell Ford <russell.ford@nyu.edu>
 *                         Menglei Zhang <menglei@nyu.edu>
 *
 *       Modified by: Tommaso Zugno <tommasozugno@gmail.com>
 *                               Integration of Carrier Aggregation
 */

#include "mmwave-enb-net-device.h"

#include "encode_e2apv1.hpp"
#include "mmwave-net-device.h"
#include "mmwave-ue-net-device.h"
#include "zlib.h"

#include <ns3/abort.h>
#include <ns3/base64.h>
#include <ns3/cJSON.h>
#include <ns3/callback.h>
#include <ns3/enum.h>
#include <ns3/epc-x2.h>
#include <ns3/ipv4-l3-protocol.h>
#include <ns3/ipv6-l3-protocol.h>
#include <ns3/llc-snap-header.h>
#include <ns3/log.h>
#include <ns3/lte-enb-component-carrier-manager.h>
#include <ns3/lte-enb-rrc.h>
#include <ns3/lte-pdcp-tag.h>
#include <ns3/mmwave-component-carrier-enb.h>
#include <ns3/node.h>
#include <ns3/packet-burst.h>
#include <ns3/packet.h>
#include <ns3/pointer.h>
#include <ns3/simulator.h>
#include <ns3/trace-source-accessor.h>
#include <ns3/uinteger.h>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("MmWaveEnbNetDevice");

namespace mmwave
{

NS_OBJECT_ENSURE_REGISTERED(MmWaveEnbNetDevice);

TypeId
MmWaveEnbNetDevice::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::MmWaveEnbNetDevice")
            .SetParent<MmWaveNetDevice>()
            .AddConstructor<MmWaveEnbNetDevice>()
            .AddAttribute("LteEnbComponentCarrierManager",
                          "The ComponentCarrierManager associated to this EnbNetDevice",
                          PointerValue(),
                          MakePointerAccessor(&MmWaveEnbNetDevice::m_componentCarrierManager),
                          MakePointerChecker<LteEnbComponentCarrierManager>())
            .AddAttribute("LteEnbRrc",
                          "The RRC layer associated with the ENB",
                          PointerValue(),
                          MakePointerAccessor(&MmWaveEnbNetDevice::m_rrc),
                          MakePointerChecker<LteEnbRrc>())
            .AddAttribute("CellId",
                          "Cell Identifier",
                          UintegerValue(0),
                          MakeUintegerAccessor(&MmWaveEnbNetDevice::m_cellId),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("E2Termination",
                          "The E2 termination object associated to this node",
                          PointerValue(),
                          MakePointerAccessor(&MmWaveEnbNetDevice::SetE2Termination,
                                              &MmWaveEnbNetDevice::GetE2Termination),
                          MakePointerChecker<E2Termination>())
            .AddAttribute("E2ReportPeriod",
                          "Periodicity of E2 reporting (value in seconds)",
                          DoubleValue(0.5),
                          MakeDoubleAccessor(&MmWaveEnbNetDevice::m_e2ReportPeriod),
                          MakeDoubleChecker<double>())
            .AddAttribute("RlcCalculator",
                          "The RLC calculator object for reporting",
                          PointerValue(),
                          MakePointerAccessor(&MmWaveEnbNetDevice::m_rlcStatsCalculator),
                          MakePointerChecker<MmWaveBearerStatsCalculator>())
            .AddAttribute("UpLinkPhyCalculator",
                          "The DU calculator object for reporting",
                          PointerValue(),
                          MakePointerAccessor(&MmWaveEnbNetDevice::m_upLinkPhyCalculator),
                          MakePointerChecker<MmWavePhyTrace>())
            .AddAttribute("DownLinkPhyCalculator",
                          "The DU calculator object for reporting",
                          PointerValue(),
                          MakePointerAccessor(&MmWaveEnbNetDevice::m_downLinkPhyCalculator),
                          MakePointerChecker<MmWavePhyTrace>())
            .AddAttribute("PdcpCalculator",
                          "The PDCP calculator object for  reporting",
                          PointerValue(),
                          MakePointerAccessor(&MmWaveEnbNetDevice::m_pdcpStatsCalculator),
                          MakePointerChecker<MmWaveBearerStatsCalculator>());
    return tid;
}

MmWaveEnbNetDevice::MmWaveEnbNetDevice()
    //: m_cellId(0),
    // m_Bandwidth (72),
    // m_Earfcn(1),
    : m_componentCarrierManager(0),
      m_isConfigured(false),
      m_clientFd(-1)
{
    NS_LOG_FUNCTION(this);
}

MmWaveEnbNetDevice::~MmWaveEnbNetDevice()
{
    NS_LOG_FUNCTION(this);
}

void
MmWaveEnbNetDevice::DoInitialize(void)
{
    NS_LOG_FUNCTION(this);
    m_isConstructed = true;
    UpdateConfig();
    for (auto it = m_ccMap.begin(); it != m_ccMap.end(); ++it)
    {
        it->second->Initialize();
    }
    m_rrc->Initialize();
    m_componentCarrierManager->Initialize();
}

void
MmWaveEnbNetDevice::DoDispose()
{
    NS_LOG_FUNCTION(this);

    m_rrc->Dispose();
    m_rrc = 0;

    m_componentCarrierManager->Dispose();
    m_componentCarrierManager = 0;
    // MmWaveComponentCarrierEnb::DoDispose() will call DoDispose
    // of its PHY, MAC, FFR and scheduler instance
    for (uint32_t i = 0; i < m_ccMap.size(); i++)
    {
        m_ccMap.at(i)->Dispose();
        m_ccMap.at(i) = 0;
    }

    MmWaveNetDevice::DoDispose();
}

Ptr<MmWaveEnbPhy>
MmWaveEnbNetDevice::GetPhy(void) const
{
    NS_LOG_FUNCTION(this);
    return DynamicCast<MmWaveComponentCarrierEnb>(m_ccMap.at(0))->GetPhy();
}

Ptr<MmWaveEnbPhy>
MmWaveEnbNetDevice::GetPhy(uint8_t index)
{
    return DynamicCast<MmWaveComponentCarrierEnb>(m_ccMap.at(index))->GetPhy();
}

uint16_t
MmWaveEnbNetDevice::GetCellId() const
{
    NS_LOG_FUNCTION(this);
    return m_cellId;
}

bool
MmWaveEnbNetDevice::HasCellId(uint16_t cellId) const
{
    for (auto& it : m_ccMap)
    {
        if (DynamicCast<MmWaveComponentCarrierEnb>(it.second)->GetCellId() == cellId)
        {
            return true;
        }
    }
    return false;
}

uint8_t
MmWaveEnbNetDevice::GetBandwidth() const
{
    NS_LOG_FUNCTION(this);
    return m_Bandwidth;
}

void
MmWaveEnbNetDevice::SetBandwidth(uint8_t bw)
{
    NS_LOG_FUNCTION(this << bw);
    m_Bandwidth = bw;
}

Ptr<MmWaveEnbMac>
MmWaveEnbNetDevice::GetMac(void)
{
    return DynamicCast<MmWaveComponentCarrierEnb>(m_ccMap.at(0))->GetMac();
}

Ptr<MmWaveEnbMac>
MmWaveEnbNetDevice::GetMac(uint8_t index)
{
    return DynamicCast<MmWaveComponentCarrierEnb>(m_ccMap.at(index))->GetMac();
}

void
MmWaveEnbNetDevice::SetRrc(Ptr<LteEnbRrc> rrc)
{
    m_rrc = rrc;
}

Ptr<LteEnbRrc>
MmWaveEnbNetDevice::GetRrc(void)
{
    return m_rrc;
}

Ptr<E2Termination>
MmWaveEnbNetDevice::GetE2Termination() const
{
    return m_e2term;
}

void
MmWaveEnbNetDevice::SetE2Termination(Ptr<E2Termination> e2term)
{
    m_e2term = e2term;

    NS_LOG_DEBUG("Register E2SM");

    Ptr<KpmFunctionDescription> kpmFd = Create<KpmFunctionDescription>();
    e2term->RegisterKpmCallbackToE2Sm(
        200,
        kpmFd,
        std::bind(&MmWaveEnbNetDevice::KpmSubscriptionCallback, this, std::placeholders::_1));

    // if (!m_forceE2FileLogging)
    // {
    //     Ptr<KpmFunctionDescription> kpmFd = Create<KpmFunctionDescription>();
    //     e2term->RegisterKpmCallbackToE2Sm(
    //         200,
    //         kpmFd,
    //         std::bind(&MmWaveEnbNetDevice::KpmSubscriptionCallback, this,
    //         std::placeholders::_1));
    // }
}

void
MmWaveEnbNetDevice::SetClientFd(int clientFd)
{
    m_clientFd = clientFd;

    Simulator::ScheduleWithContext(GetNode()->GetId(),
                                   MilliSeconds(100),
                                   &MmWaveEnbNetDevice::BuildAndSendReportMessage,
                                   this);
}

int
MmWaveEnbNetDevice::GetClientFd() const
{
    return m_clientFd;
}

/**
 * KPM Subscription Request callback.
 * This function is triggered whenever a RIC Subscription Request for
 * the KPM RAN Function is received.
 *
 * \param pdu request message
 */
void
MmWaveEnbNetDevice::KpmSubscriptionCallback(E2AP_PDU_t* sub_req_pdu)
{
    NS_LOG_DEBUG("\nReceived RIC Subscription Request, cellId= " << m_cellId << "\n");

    E2Termination::RicSubscriptionRequest_rval_s params =
        m_e2term->ProcessRicSubscriptionRequest(sub_req_pdu);
    NS_LOG_DEBUG("requestorId " << +params.requestorId << ", instanceId " << +params.instanceId
                                << ", ranFuncionId " << +params.ranFuncionId << ", actionId "
                                << +params.actionId);
    BuildAndSendReportMessage();
    // if (!m_isReportingEnabled)
    // {
    //     BuildAndSendReportMessage(params);
    //     m_isReportingEnabled = true;
    // }
}

Ptr<KpmIndicationHeader>
MmWaveEnbNetDevice::BuildRicIndicationHeader(std::string plmId,
                                             std::string gnbId,
                                             uint16_t nrCellId)
{
    KpmIndicationHeader::KpmRicIndicationHeaderValues headerValues;
    headerValues.m_plmId = plmId;
    headerValues.m_gnbId = gnbId;
    headerValues.m_nrCellId = nrCellId;
    auto time = Simulator::Now();
    // uint64_t timestamp = m_startTime + (uint64_t)time.GetMilliSeconds();
    uint64_t timestamp = (uint64_t)time.GetMilliSeconds();
    NS_LOG_DEBUG("NR plmid " << plmId << " gnbId " << gnbId << " nrCellId " << nrCellId);
    NS_LOG_DEBUG("Timestamp " << timestamp);
    headerValues.m_timestamp = timestamp;

    Ptr<KpmIndicationHeader> header =
        Create<KpmIndicationHeader>(KpmIndicationHeader::GlobalE2nodeType::gNB, headerValues);

    return header;
}

void
MmWaveEnbNetDevice::ControlMessageReceivedCallback(E2AP_PDU_t* pdu)
{
    // NS_LOG_DEBUG ("Control Message Received, cellId is " << m_gnbNetDev->GetCellId ());
    // std::cout << "Control Message Received, cellId is " << this->GetCellId() << std::endl;
    InitiatingMessage_t* mess = pdu->choice.initiatingMessage;
    auto* request = (RICcontrolRequest_t*)&mess->value.choice.RICcontrolRequest;
    NS_LOG_INFO(xer_fprint(stderr, &asn_DEF_RICcontrolRequest, request));

    size_t count = request->protocolIEs.list.count;
    if (count <= 0)
    {
        NS_LOG_ERROR("[E2SM] received empty list");
        return;
    }
    for (size_t i = 0; i < count; i++)
    {
        RICcontrolRequest_IEs_t* ie = request->protocolIEs.list.array[i];
        switch (ie->value.present)
        {
        case RICcontrolRequest_IEs__value_PR_RICcontrolMessage: {
            NS_LOG_DEBUG("Control Message: " << ie->value.choice.RICcontrolMessage.buf);
            // cJSON *json = cJSON_Parse ((const char *) ie->value.choice.RICcontrolMessage.buf);
            // if (json == NULL)
            //   {
            //     NS_LOG_ERROR ("Parsing json failed");
            //   }
            // else
            //   {
            //     // NS_LOG_DEBUG("Get available json");
            //     cJSON *uePolicy = NULL;
            //     cJSON_ArrayForEach (uePolicy, json)
            //     {
            //       cJSON *imsi = cJSON_GetObjectItemCaseSensitive (uePolicy, "imsi");
            //       cJSON *cfNodeId = cJSON_GetObjectItemCaseSensitive (uePolicy, "cfNodeId");
            //       if (cJSON_IsNumber (imsi) && cJSON_IsNumber (cfNodeId))
            //         {
            //           NS_LOG_DEBUG ("Recv ctrl policy for imsi "
            //                         << imsi->valueint << " to cell " << m_gnbNetDev->GetCellId ()
            //                         << " to cfNode " << cfNodeId->valueint);
            //           m_vrSystemProfile->SetProfileForImsi (
            //               imsi->valueint, m_gnbNetDev->GetCellId (), cfNodeId->valueint);
            //           // m_vrSystemControl->ApplyForSingleUe(imsi->valueint,
            //           m_gnbNetDev->GetCellId(), cfNodeId->valueint);
            //         }
            //     }
            //   }
            break;
        }
        default:
            break;
        }
    }
}

std::string
MmWaveEnbNetDevice::GetImsiString(uint64_t imsi)
{
    std::string ueImsi = std::to_string(imsi);
    std::string ueImsiComplete{};
    if (ueImsi.length() == 1)
    {
        ueImsiComplete = "0000" + ueImsi;
    }
    else if (ueImsi.length() == 2)
    {
        ueImsiComplete = "000" + ueImsi;
    }
    else
    {
        ueImsiComplete = "00" + ueImsi;
    }
    return ueImsiComplete;
}

void
MmWaveEnbNetDevice::BuildAndSendReportMessage()
{
    NS_LOG_FUNCTION(this);
    std::string plmId = "111";
    std::string gnbId = std::to_string(m_cellId);

    cJSON* msg = cJSON_CreateObject();
    cJSON_AddStringToObject(msg, "msgSource", "MmwaveEnbNetDev");
    cJSON_AddNumberToObject(msg, "cellId", m_cellId);
    cJSON_AddNumberToObject(msg, "updateTime", Simulator::Now().GetSeconds());
    cJSON_AddStringToObject(msg, "msgType", "KpmIndication");

    cJSON* gnbPos = cJSON_AddObjectToObject(msg, "pos");
    Vector pos = this->GetNode()->GetObject<MobilityModel>()->GetPosition();
    cJSON_AddNumberToObject(gnbPos, "x", pos.x);
    cJSON_AddNumberToObject(gnbPos, "y", pos.y);
    cJSON_AddNumberToObject(gnbPos, "z", pos.z);

    auto ueMap = m_rrc->GetUeMap();

    NS_LOG_DEBUG("Cell " << m_cellId << " UeMap size " << ueMap.size());
    uint32_t macPduCellSpecific = 0;

    cJSON* ueMsgArray = cJSON_AddArrayToObject(msg, "UeMsgArray");

    for (auto ue : ueMap)
    {
        uint64_t imsi = ue.second->GetImsi();
        std::string ueImsiComplete = GetImsiString(imsi);
        uint16_t rnti = ue.second->GetRnti();

        uint32_t macPduUe = m_upLinkPhyCalculator->GetMacPduUeSpecific(rnti, m_cellId);

        macPduCellSpecific += macPduUe;

        // Numerator = (Sum of number of symbols across all rows (TTIs) group by cell ID and UE ID
        // within a given time window)
        double macUplinkNumberOfSymbols =
            m_upLinkPhyCalculator->GetMacNumberOfSymbolsUeSpecific(rnti, m_cellId);
        double macDownlinkNumberOfSymbols =
            m_downLinkPhyCalculator->GetMacNumberOfSymbolsUeSpecific(rnti, m_cellId);

        NS_LOG_DEBUG("UE " << imsi << " uplink symbols " << +macUplinkNumberOfSymbols);
        NS_LOG_DEBUG("UE " << imsi << " downlink symbols " << +macDownlinkNumberOfSymbols);

        auto phyMac = GetMac()->GetConfigurationParameters();
        // Denominator = (Periodicity of the report time window in ms*number of TTIs per ms*14)
        Time reportingWindow =
            Simulator::Now() - m_upLinkPhyCalculator->GetLastResetTime(rnti, m_cellId);
        double denominatorPrb =
            std::ceil(reportingWindow.GetNanoSeconds() / phyMac->GetSlotPeriod().GetNanoSeconds()) *
            14;
        m_upLinkPhyCalculator->ResetPhyTracesForRntiCellId(rnti, m_cellId);
        m_downLinkPhyCalculator->ResetPhyTracesForRntiCellId(rnti, m_cellId);
        NS_LOG_DEBUG("macNumberOfSymbols " << macUplinkNumberOfSymbols + macDownlinkNumberOfSymbols
                                           << " denominatorPrb " << denominatorPrb);

        double periodTime =
            (Simulator::Now() - m_rlcStatsCalculator->GetLastImsiLcidResetTime(imsi, 3))
                .GetMilliSeconds();
        double uplinkRlcLatency = m_rlcStatsCalculator->GetUlDelay(imsi, 3) / 1e6;
        double uplinkpduStats =
            // m_rlcStatsCalculator->GetUlRxData(imsi, 3) * 8.0 / 1e3; // unit kbit
            m_rlcStatsCalculator->GetUlPduSizeStats(imsi, 3)[0] * 8.0 / 1e3;            // unit kbit
        double uplinkDataSize = m_rlcStatsCalculator->GetUlRxData(imsi, 3) * 8.0 / 1e3; // unit kbit
        double uplinkRlcBitrate =
            (uplinkRlcLatency == 0) ? 0 : uplinkpduStats / uplinkRlcLatency; // unit kbit/s
        double uplinkThroughput = uplinkDataSize / periodTime;

        double downlinkRlcLatency = m_rlcStatsCalculator->GetDlDelay(imsi, 3) / 1e6;
        double downlinkpduStats =
            m_rlcStatsCalculator->GetDlPduSizeStats(imsi, 3)[0] * 8.0 / 1e3; // unit kbit
        double downlinkDataSize =
            m_rlcStatsCalculator->GetDlRxData(imsi, 3) * 8.0 / 1e3; // unit kbit
        double downlinkRlcBitrate =
            (downlinkRlcLatency == 0) ? 0 : downlinkpduStats / downlinkRlcLatency; // unit kbit/s
        double downlinkThroughput = downlinkDataSize / periodTime;

        m_rlcStatsCalculator->ResetResultsForImsiLcid(imsi, 3);
        NS_LOG_DEBUG("Period Time: " << periodTime << "s");
        NS_LOG_DEBUG("uplink RLC latency " << uplinkRlcLatency << "ms "
                                           << " dataSize " << uplinkDataSize << "kbit "
                                           << " dataRate " << uplinkRlcBitrate << "Mbps"
                                           << " throughput " << uplinkThroughput << "Mbps");
        NS_LOG_DEBUG("downlink RLC latency " << downlinkRlcLatency << "ms "
                                             << " dataSize " << downlinkDataSize << "kbit "
                                             << " dataRate " << downlinkRlcBitrate << "Mbps"
                                             << " throughput " << downlinkThroughput << "Mbps");

        cJSON* ueMsg = cJSON_CreateObject();
        cJSON_AddNumberToObject(ueMsg, "imsi", imsi);
        cJSON_AddNumberToObject(ueMsg, "upPrb", macUplinkNumberOfSymbols);
        cJSON_AddNumberToObject(ueMsg, "downPrb", macDownlinkNumberOfSymbols);
        cJSON_AddNumberToObject(ueMsg, "upRlcLatency", uplinkRlcLatency);
        cJSON_AddNumberToObject(ueMsg, "upThroughput", uplinkThroughput);
        cJSON_AddNumberToObject(ueMsg, "upRlcSize", uplinkDataSize);
        cJSON_AddNumberToObject(ueMsg, "downRlcLatency", downlinkRlcLatency);
        cJSON_AddNumberToObject(ueMsg, "downRlcSize", downlinkDataSize);
        cJSON_AddNumberToObject(ueMsg, "downThroughput", downlinkThroughput);

        cJSON_AddItemToArray(ueMsgArray, ueMsg);
    }

    std::string cJsonPrint = cJSON_PrintUnformatted(msg);
    const char* reportString = cJsonPrint.c_str();
    // const char* testString = cJSON_PrintUnformatted(ueStatsMsg).c_str();
    // NS_LOG_DEBUG(testString.c_str());
    char b[2048];

    z_stream defstream;
    defstream.zalloc = Z_NULL;
    defstream.zfree = Z_NULL;
    defstream.opaque = Z_NULL;
    // setup "a" as the input and "b" as the compressed output
    defstream.avail_in = (uInt)strlen(reportString) + 1; // size of input, string + terminator
    defstream.next_in = (Bytef*)reportString;            // input char array
    defstream.avail_out = (uInt)sizeof(b);               // size of output
    defstream.next_out = (Bytef*)b;                      // output char array

    // the actual compression work.
    deflateInit(&defstream, Z_BEST_COMPRESSION);
    deflate(&defstream, Z_FINISH);
    deflateEnd(&defstream);

    // NS_LOG_DEBUG("Original len: " << strlen(reportString));
    // // This is one way of getting the size of the output
    // printf("Compressed size is: %lu\n", defstream.total_out);
    // printf("Compressed size(wrong) is: %lu\n", strlen(b));
    // printf("Compressed string is: %s\n", b);

    std::string base64String = base64_encode((const unsigned char*)b, defstream.total_out);
    // NS_LOG_DEBUG("base64String: " << base64String);
    // NS_LOG_DEBUG("base64String size: " << base64String.length());

    if (m_e2term != nullptr)
    {
        Ptr<KpmIndicationHeader> header = BuildRicIndicationHeader(plmId, gnbId, m_cellId);
        E2AP_PDU* pdu_du_ue = new E2AP_PDU;
        auto kpmPrams = m_e2term->GetSubscriptionPara();
        NS_LOG_DEBUG("kpmPrams.ranFuncionId: " << kpmPrams.ranFuncionId);
        encoding::generate_e2apv1_indication_request_parameterized(
            pdu_du_ue,
            kpmPrams.requestorId,
            kpmPrams.instanceId,
            kpmPrams.ranFuncionId,
            kpmPrams.actionId,
            1,                          // TODO sequence number
            (uint8_t*)header->m_buffer, // buffer containing the encoded header
            header->m_size,             // size of the encoded header
            //  (uint8_t*) duMsg->m_buffer, // buffer containing the encoded message
            // (uint8_t*)(reportString),
            (uint8_t*)base64String.c_str(),
            //  duMsg->m_size
            base64String.length());
        // defstream.total_out); // size of the encoded message
        m_e2term->SendE2Message(pdu_du_ue);
        delete pdu_du_ue;
    }
    else if (m_clientFd > 0)
    {
        if (send(m_clientFd, base64String.c_str(), base64String.length(), 0) < 0)
        {
            NS_LOG_ERROR("Error when send cell data.");
        }
    }

    NS_LOG_DEBUG("MmWaveEnbNetDevice " << m_cellId << " send indication messsage");
    
    Simulator::ScheduleWithContext(GetNode()->GetId(),
                                   Seconds(m_e2ReportPeriod),
                                   &MmWaveEnbNetDevice::BuildAndSendReportMessage,
                                   this);
}

bool
MmWaveEnbNetDevice::DoSend(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
    NS_LOG_FUNCTION(this << packet << dest << protocolNumber);
    NS_ABORT_MSG_IF(protocolNumber != Ipv4L3Protocol::PROT_NUMBER &&
                        protocolNumber != Ipv6L3Protocol::PROT_NUMBER,
                    "unsupported protocol " << protocolNumber << ", only IPv4/IPv6 is supported");
    return m_rrc->SendData(packet);
    // EpcX2SapUser::UeDataParams params;

    // params.gtpTeid = 1;
    // params.ueData = packet;
    // PdcpTag pdcpTag(Simulator::Now());

    // params.ueData->AddByteTag(pdcpTag);
    // this->GetNode()->GetObject<EpcX2>()->GetX2RlcUserMap().find(1)->second->SendMcPdcpSdu(params);
}

void
MmWaveEnbNetDevice::UpdateConfig(void)
{
    NS_LOG_FUNCTION(this);

    if (m_isConstructed)
    {
        if (!m_isConfigured)
        {
            NS_LOG_LOGIC(this << " Configure cell " << m_cellId);
            // we have to make sure that this function is called only once
            // m_rrc->ConfigureCell (m_Bandwidth, m_Bandwidth, m_Earfcn, m_Earfcn, m_cellId);
            NS_ASSERT(!m_ccMap.empty());

            // create the MmWaveComponentCarrierConf map used for the RRC setup
            std::map<uint8_t, LteEnbRrc::MmWaveComponentCarrierConf> ccConfMap;
            for (auto it = m_ccMap.begin(); it != m_ccMap.end(); ++it)
            {
                Ptr<MmWaveComponentCarrierEnb> ccEnb =
                    DynamicCast<MmWaveComponentCarrierEnb>(it->second);
                LteEnbRrc::MmWaveComponentCarrierConf ccConf;
                ccConf.m_ccId = ccEnb->GetConfigurationParameters()->GetCcId();
                ccConf.m_cellId = ccEnb->GetCellId();
                ccConf.m_bandwidth = ccEnb->GetBandwidthInRb();

                ccConfMap[it->first] = ccConf;
            }

            m_rrc->ConfigureCell(ccConfMap);

            if (m_e2term != nullptr)
            {
                NS_LOG_DEBUG("E2sim start in cell " << m_cellId);
                // Simulator::Schedule(MicroSeconds(0), &E2Termination::Start, m_e2term);
                Simulator::ScheduleWithContext(GetNode()->GetId(),
                                               MicroSeconds(0),
                                               &E2Termination::Start,
                                               m_e2term);
            }
            m_isConfigured = true;
        }

        // m_rrc->SetCsgId (m_csgId, m_csgIndication);
    }
    else
    {
        /*
         * Lower layers are not ready yet, so do nothing now and expect
         * ``DoInitialize`` to re-invoke this function.
         */
    }
}

void
MmWaveEnbNetDevice::SetCcMap(std::map<uint8_t, Ptr<MmWaveComponentCarrier>> ccm)
{
    NS_ASSERT_MSG(!m_isConfigured, "attempt to set CC map after configuration");
    m_ccMap = ccm;
}

} // namespace mmwave
} // namespace ns3
