/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 TELEMATICS LAB, DEE - Politecnico di Bari
 *
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
 * Author: Giuseppe Piro  <g.piro@poliba.it>
 * Author: Marco Miozzo <mmiozzo@cttc.es> : Update to FF API Architecture
 * Author: Nicola Baldo <nbaldo@cttc.es>  : Integrated with new RRC and MAC architecture
 * Author: Danilo Abrignani <danilo.abrignani@unibo.it> : Integrated with new architecture - GSoC
 * 2015 - Carrier Aggregation
 *
 * Modified by: Michele Polese <michele.polese@gmail.com>
 *          Dual Connectivity functionalities
 */

#include "encode_e2apv1.hpp"
#include "zlib.h"

#include <ns3/abort.h>
#include <ns3/base64.h>
#include <ns3/cJSON.h>
#include <ns3/callback.h>
#include <ns3/enum.h>
#include <ns3/ff-mac-scheduler.h>
#include <ns3/ipv4-l3-protocol.h>
#include <ns3/ipv6-l3-protocol.h>
#include <ns3/llc-snap-header.h>
#include <ns3/log.h>
#include <ns3/lte-amc.h>
#include <ns3/lte-anr.h>
#include <ns3/lte-enb-component-carrier-manager.h>
#include <ns3/lte-enb-mac.h>
#include <ns3/lte-enb-net-device.h>
#include <ns3/lte-enb-phy.h>
#include <ns3/lte-enb-rrc.h>
#include <ns3/lte-ffr-algorithm.h>
#include <ns3/lte-handover-algorithm.h>
#include <ns3/lte-net-device.h>
#include <ns3/lte-ue-net-device.h>
#include <ns3/node.h>
#include <ns3/object-factory.h>
#include <ns3/object-map.h>
#include <ns3/packet-burst.h>
#include <ns3/packet.h>
#include <ns3/pointer.h>
#include <ns3/simulator.h>
#include <ns3/trace-source-accessor.h>
#include <ns3/uinteger.h>

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("LteEnbNetDevice");

NS_OBJECT_ENSURE_REGISTERED(LteEnbNetDevice);

TypeId
LteEnbNetDevice::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::LteEnbNetDevice")
            .SetParent<LteNetDevice>()
            .AddConstructor<LteEnbNetDevice>()
            .AddAttribute("LteEnbRrc",
                          "The RRC associated to this EnbNetDevice",
                          PointerValue(),
                          MakePointerAccessor(&LteEnbNetDevice::m_rrc),
                          MakePointerChecker<LteEnbRrc>())
            .AddAttribute("LteHandoverAlgorithm",
                          "The handover algorithm associated to this EnbNetDevice",
                          PointerValue(),
                          MakePointerAccessor(&LteEnbNetDevice::m_handoverAlgorithm),
                          MakePointerChecker<LteHandoverAlgorithm>())
            .AddAttribute(
                "LteAnr",
                "The automatic neighbour relation function associated to this EnbNetDevice",
                PointerValue(),
                MakePointerAccessor(&LteEnbNetDevice::m_anr),
                MakePointerChecker<LteAnr>())
            .AddAttribute("LteFfrAlgorithm",
                          "The FFR algorithm associated to this EnbNetDevice",
                          PointerValue(),
                          MakePointerAccessor(&LteEnbNetDevice::m_ffrAlgorithm),
                          MakePointerChecker<LteFfrAlgorithm>())
            .AddAttribute("LteEnbComponentCarrierManager",
                          "The RRC associated to this EnbNetDevice",
                          PointerValue(),
                          MakePointerAccessor(&LteEnbNetDevice::m_componentCarrierManager),
                          MakePointerChecker<LteEnbComponentCarrierManager>())
            .AddAttribute("ComponentCarrierMap",
                          "List of component carriers.",
                          ObjectMapValue(),
                          MakeObjectMapAccessor(&LteEnbNetDevice::m_ccMap),
                          MakeObjectMapChecker<ComponentCarrierEnb>())
            .AddAttribute(
                "UlBandwidth",
                "Uplink Transmission Bandwidth Configuration in number of Resource Blocks",
                UintegerValue(100),
                MakeUintegerAccessor(&LteEnbNetDevice::SetUlBandwidth,
                                     &LteEnbNetDevice::GetUlBandwidth),
                MakeUintegerChecker<uint8_t>())
            .AddAttribute(
                "DlBandwidth",
                "Downlink Transmission Bandwidth Configuration in number of Resource Blocks",
                UintegerValue(100),
                MakeUintegerAccessor(&LteEnbNetDevice::SetDlBandwidth,
                                     &LteEnbNetDevice::GetDlBandwidth),
                MakeUintegerChecker<uint8_t>())
            .AddAttribute("CellId",
                          "Cell Identifier",
                          UintegerValue(0),
                          MakeUintegerAccessor(&LteEnbNetDevice::m_cellId),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("DlEarfcn",
                          "Downlink E-UTRA Absolute Radio Frequency Channel Number (EARFCN) "
                          "as per 3GPP 36.101 Section 5.7.3. ",
                          UintegerValue(100),
                          MakeUintegerAccessor(&LteEnbNetDevice::m_dlEarfcn),
                          MakeUintegerChecker<uint32_t>(0, 262143))
            .AddAttribute("UlEarfcn",
                          "Uplink E-UTRA Absolute Radio Frequency Channel Number (EARFCN) "
                          "as per 3GPP 36.101 Section 5.7.3. ",
                          UintegerValue(18100),
                          MakeUintegerAccessor(&LteEnbNetDevice::m_ulEarfcn),
                          MakeUintegerChecker<uint32_t>(0, 262143))
            .AddAttribute(
                "CsgId",
                "The Closed Subscriber Group (CSG) identity that this eNodeB belongs to",
                UintegerValue(0),
                MakeUintegerAccessor(&LteEnbNetDevice::SetCsgId, &LteEnbNetDevice::GetCsgId),
                MakeUintegerChecker<uint32_t>())
            .AddAttribute(
                "CsgIndication",
                "If true, only UEs which are members of the CSG (i.e. same CSG ID) "
                "can gain access to the eNodeB, therefore enforcing closed access mode. "
                "Otherwise, the eNodeB operates as a non-CSG cell and implements open access mode.",
                BooleanValue(false),
                MakeBooleanAccessor(&LteEnbNetDevice::SetCsgIndication,
                                    &LteEnbNetDevice::GetCsgIndication),
                MakeBooleanChecker())
            .AddAttribute("E2Termination",
                          "The E2 termination object associated to this node",
                          PointerValue(),
                          MakePointerAccessor(&LteEnbNetDevice::SetE2Termination,
                                              &LteEnbNetDevice::GetE2Termination),
                          MakePointerChecker<E2Termination>());
    return tid;
}

LteEnbNetDevice::LteEnbNetDevice()
    : m_isConstructed(false),
      m_isConfigured(false),
      m_anr(0),
      m_componentCarrierManager(0),
      m_clientFd(-1)
{
    NS_LOG_FUNCTION(this);
}

LteEnbNetDevice::~LteEnbNetDevice(void)
{
    NS_LOG_FUNCTION(this);
}

void
LteEnbNetDevice::DoDispose()
{
    NS_LOG_FUNCTION(this);

    m_rrc->Dispose();
    m_rrc = 0;

    m_handoverAlgorithm->Dispose();
    m_handoverAlgorithm = 0;

    if (m_anr)
    {
        m_anr->Dispose();
        m_anr = 0;
    }
    m_componentCarrierManager->Dispose();
    m_componentCarrierManager = 0;
    // ComponentCarrierEnb::DoDispose() will call DoDispose
    // of its PHY, MAC, FFR and scheduler instance
    for (uint32_t i = 0; i < m_ccMap.size(); i++)
    {
        m_ccMap.at(i)->Dispose();
        m_ccMap.at(i) = 0;
    }

    LteNetDevice::DoDispose();
}

Ptr<LteEnbMac>
LteEnbNetDevice::GetMac() const
{
    return m_ccMap.at(0)->GetMac();
}

Ptr<LteEnbPhy>
LteEnbNetDevice::GetPhy() const
{
    return m_ccMap.at(0)->GetPhy();
}

Ptr<LteEnbMac>
LteEnbNetDevice::GetMac(uint8_t index)
{
    return m_ccMap.at(index)->GetMac();
}

Ptr<LteEnbPhy>
LteEnbNetDevice::GetPhy(uint8_t index)
{
    return m_ccMap.at(index)->GetPhy();
}

Ptr<LteEnbRrc>
LteEnbNetDevice::GetRrc() const
{
    return m_rrc;
}

Ptr<LteEnbComponentCarrierManager>
LteEnbNetDevice::GetComponentCarrierManager() const
{
    return m_componentCarrierManager;
}

uint16_t
LteEnbNetDevice::GetCellId() const
{
    return m_cellId;
}

bool
LteEnbNetDevice::HasCellId(uint16_t cellId) const
{
    for (auto& it : m_ccMap)
    {
        if (it.second->GetCellId() == cellId)
        {
            return true;
        }
    }
    return false;
}

uint8_t
LteEnbNetDevice::GetUlBandwidth() const
{
    return m_ulBandwidth;
}

void
LteEnbNetDevice::SetUlBandwidth(uint8_t bw)
{
    NS_LOG_FUNCTION(this << uint16_t(bw));
    switch (bw)
    {
    case 6:
    case 15:
    case 25:
    case 50:
    case 75:
    case 100:
        m_ulBandwidth = bw;
        break;

    default:
        NS_FATAL_ERROR("invalid bandwidth value " << (uint16_t)bw);
        break;
    }
}

uint8_t
LteEnbNetDevice::GetDlBandwidth() const
{
    return m_dlBandwidth;
}

void
LteEnbNetDevice::SetDlBandwidth(uint8_t bw)
{
    NS_LOG_FUNCTION(this << uint16_t(bw));
    switch (bw)
    {
    case 6:
    case 15:
    case 25:
    case 50:
    case 75:
    case 100:
        m_dlBandwidth = bw;
        break;

    default:
        NS_FATAL_ERROR("invalid bandwidth value " << (uint16_t)bw);
        break;
    }
}

uint32_t
LteEnbNetDevice::GetDlEarfcn() const
{
    return m_dlEarfcn;
}

void
LteEnbNetDevice::SetDlEarfcn(uint32_t earfcn)
{
    NS_LOG_FUNCTION(this << earfcn);
    m_dlEarfcn = earfcn;
}

uint32_t
LteEnbNetDevice::GetUlEarfcn() const
{
    return m_ulEarfcn;
}

void
LteEnbNetDevice::SetUlEarfcn(uint32_t earfcn)
{
    NS_LOG_FUNCTION(this << earfcn);
    m_ulEarfcn = earfcn;
}

uint32_t
LteEnbNetDevice::GetCsgId() const
{
    return m_csgId;
}

void
LteEnbNetDevice::SetCsgId(uint32_t csgId)
{
    NS_LOG_FUNCTION(this << csgId);
    m_csgId = csgId;
    UpdateConfig(); // propagate the change to RRC level
}

bool
LteEnbNetDevice::GetCsgIndication() const
{
    return m_csgIndication;
}

void
LteEnbNetDevice::SetCsgIndication(bool csgIndication)
{
    NS_LOG_FUNCTION(this << csgIndication);
    m_csgIndication = csgIndication;
    UpdateConfig(); // propagate the change to RRC level
}

std::map<uint8_t, Ptr<ComponentCarrierEnb>>
LteEnbNetDevice::GetCcMap()
{
    return m_ccMap;
}

void
LteEnbNetDevice::SetCcMap(std::map<uint8_t, Ptr<ComponentCarrierEnb>> ccm)
{
    NS_ASSERT_MSG(!m_isConfigured, "attempt to set CC map after configuration");
    m_ccMap = ccm;
}

void
LteEnbNetDevice::DoInitialize(void)
{
    NS_LOG_FUNCTION(this);
    m_isConstructed = true;
    UpdateConfig();
    std::map<uint8_t, Ptr<ComponentCarrierEnb>>::iterator it;
    for (it = m_ccMap.begin(); it != m_ccMap.end(); ++it)
    {
        it->second->Initialize();
    }
    m_rrc->Initialize();
    m_componentCarrierManager->Initialize();
    m_handoverAlgorithm->Initialize();

    if (m_anr)
    {
        m_anr->Initialize();
    }

    m_ffrAlgorithm->Initialize();
}

bool
LteEnbNetDevice::Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
    NS_LOG_FUNCTION(this << packet << dest << protocolNumber);
    NS_ABORT_MSG_IF(protocolNumber != Ipv4L3Protocol::PROT_NUMBER &&
                        protocolNumber != Ipv6L3Protocol::PROT_NUMBER,
                    "unsupported protocol " << protocolNumber
                                            << ", only IPv4 and IPv6 are supported");
    return m_rrc->SendData(packet);
}

void
LteEnbNetDevice::UpdateConfig(void)
{
    NS_LOG_FUNCTION(this);

    if (m_isConstructed)
    {
        if (!m_isConfigured)
        {
            NS_LOG_LOGIC(this << " Configure cell " << m_cellId);
            // we have to make sure that this function is called only once
            NS_ASSERT(!m_ccMap.empty());
            m_rrc->ConfigureCell(m_ccMap);
            m_isConfigured = true;
        }

        NS_LOG_LOGIC(this << " Updating SIB1 of cell " << m_cellId << " with CSG ID " << m_csgId
                          << " and CSG indication " << m_csgIndication);
        m_rrc->SetCsgId(m_csgId, m_csgIndication);

        if (m_e2term)
        {
            NS_LOG_DEBUG("E2sim start in cell " << m_cellId);
            // Simulator::Schedule(MicroSeconds(0), &E2Termination::Start, m_e2term);
            Simulator::ScheduleWithContext(GetNode()->GetId(),
                                           MicroSeconds(0),
                                           &E2Termination::Start,
                                           m_e2term);

            Ptr<RicControlFunctionDescription> ricCtrlFd = Create<RicControlFunctionDescription>();
            m_e2term->RegisterSmCallbackToE2Sm(
                300,
                ricCtrlFd,
                std::bind(&LteEnbNetDevice::ControlMessageReceivedCallback,
                          this,
                          std::placeholders::_1));
        }
    }
    else
    {
        /*
         * Lower layers are not ready yet, so do nothing now and expect
         * ``DoInitialize`` to re-invoke this function.
         */
    }
}

Ptr<E2Termination>
LteEnbNetDevice::GetE2Termination() const
{
    return m_e2term;
}

void
LteEnbNetDevice::SetE2Termination(Ptr<E2Termination> e2term)
{
    m_e2term = e2term;

    NS_LOG_DEBUG("Register E2SM");

    Ptr<KpmFunctionDescription> kpmFd = Create<KpmFunctionDescription>();
    e2term->RegisterKpmCallbackToE2Sm(
        200,
        kpmFd,
        std::bind(&LteEnbNetDevice::KpmSubscriptionCallback, this, std::placeholders::_1));
    // if (!m_forceE2FileLogging)
    // {
    // }
}

void
LteEnbNetDevice::SetClientFd(int clientFd)
{
    m_clientFd = clientFd;
    Simulator::ScheduleWithContext(GetNode()->GetId(),
                                   MilliSeconds(500),
                                   &LteEnbNetDevice::BuildAndSendReportMessage,
                                   this);
}

/**
 * KPM Subscription Request callback.
 * This function is triggerd whenever a RIC Subscription Request for
 * the KPM RAN Function is received.
 *
 * \param pdu request message
 */
void
LteEnbNetDevice::KpmSubscriptionCallback(E2AP_PDU_t* sub_req_pdu)
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

void
LteEnbNetDevice::BuildAndSendReportMessage()
{
    NS_LOG_FUNCTION(this);
    std::string plmId = "111";
    std::string gnbId = std::to_string(m_cellId);

    cJSON* msg = cJSON_CreateObject();
    cJSON_AddStringToObject(msg, "msgSource", "LteEnbNetDev");
    cJSON_AddNumberToObject(msg, "cellId", m_cellId);

    auto mmwaveImsiCellSinrMap = m_rrc->GetMmwaveImsiCellSinrMap();

    cJSON* ueMsgArray = cJSON_AddArrayToObject(msg, "UeMsgArray");
    for (auto ue : mmwaveImsiCellSinrMap)
    {
        uint64_t imsi = ue.first;
        auto cellSinrMap = ue.second;
        cJSON* ueMsg = cJSON_CreateObject();
        cJSON_AddNumberToObject(ueMsg, "imsi", imsi);

        // cJSON* ueSinr = cJSON_AddArrayToObject(ueMsg, "SINR");
        cJSON* ueSinr = cJSON_AddObjectToObject(ueMsg, "sinr");

        for (auto cell : cellSinrMap)
        {
            uint16_t cellId = cell.first;
            if (cellId == 0)
            {
                continue;
            }
            double sinr = 10 * std::log10(cell.second);
            cJSON_AddNumberToObject(ueSinr, std::to_string(cellId).c_str(), sinr);
            NS_LOG_DEBUG("imsi " << imsi << " cell " << cellId << " sinr " << sinr << "dB");
        }
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

    Simulator::ScheduleWithContext(GetNode()->GetId(),
                                   MilliSeconds(500),
                                   &LteEnbNetDevice::BuildAndSendReportMessage,
                                   this);
}

Ptr<KpmIndicationHeader>
LteEnbNetDevice::BuildRicIndicationHeader(std::string plmId, std::string gnbId, uint16_t nrCellId)
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
LteEnbNetDevice::ControlMessageReceivedCallback(E2AP_PDU_t* pdu)
{
    // NS_LOG_DEBUG ("Control Message Received, cellId is " << m_gnbNetDev->GetCellId ());
    std::cout << "Control Message Received, cellId is " << this->GetCellId() << std::endl;
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

} // namespace ns3
