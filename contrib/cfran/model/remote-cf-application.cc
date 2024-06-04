#include "remote-cf-application.h"

#include "cf-radio-header.h"
#include "multi-packet-header.h"

#include <ns3/cJSON.h>
#include <ns3/base64.h>
#include <ns3/log.h>

#include <encode_e2apv1.hpp>

#include "zlib.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("RemoteCfApplication");
NS_OBJECT_ENSURE_REGISTERED(RemoteCfApplication);

TypeId
RemoteCfApplication::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RemoteCfApplication")
                            .SetParent<CfApplication>()
                            .AddConstructor<RemoteCfApplication>();

    return tid;
}

RemoteCfApplication::RemoteCfApplication()
{
    NS_LOG_FUNCTION(this);
}

RemoteCfApplication::~RemoteCfApplication()
{
    NS_LOG_FUNCTION(this);
}

void
RemoteCfApplication::SetServerId(uint64_t serverId)
{
    m_serverId = serverId;
}

uint64_t
RemoteCfApplication::GetServerId()
{
    return m_serverId;
}

void
RemoteCfApplication::SetE2Termination(Ptr<E2Termination> e2term)
{
    m_e2term = e2term;

    NS_LOG_DEBUG("Register E2SM");

    Ptr<KpmFunctionDescription> kpmFd = Create<KpmFunctionDescription>();
    e2term->RegisterKpmCallbackToE2Sm(
        200,
        kpmFd,
        std::bind(&RemoteCfApplication::KpmSubscriptionCallback, this, std::placeholders::_1));
}

void
RemoteCfApplication::SendPacketToUe(uint64_t ueId, Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(this << ueId);
    CfranSystemInfo::UeInfo ueInfo = m_cfranSystemInfo->GetUeInfo(ueId);

    // Ipv4Address ueAddr = ueInfo.m_ipAddr;
    InetSocketAddress target = InetSocketAddress(ueInfo.m_ipAddr, ueInfo.m_port);

    m_socket->SendTo(packet, 0, target.ConvertTo());
}

void
RemoteCfApplication::RecvFromUe(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this);
    Ptr<Packet> packet;
    Address from;
    Address localAddress;

    while (packet = socket->RecvFrom(from))
    {
        socket->GetSockName(localAddress);

        CfRadioHeader cfRadioHeader;
        packet->RemoveHeader(cfRadioHeader);

        if (cfRadioHeader.GetMessageType() == CfRadioHeader::InitRequest)
        {
            NS_LOG_INFO("Remote server "
                        << "Recv init request of UE " << cfRadioHeader.GetUeId());

            auto ueId = cfRadioHeader.GetUeId();
            UpdateUeState(ueId, UeState::Initializing);

            m_cfUnit->AddNewUe(cfRadioHeader.GetUeId());

            Ptr<Packet> resultPacket = Create<Packet>(500);
            CfRadioHeader echoHeader;
            echoHeader.SetMessageType(CfRadioHeader::InitSuccess);
            echoHeader.SetGnbId(m_serverId);
            resultPacket->AddHeader(echoHeader);
            SendPacketToUe(ueId, resultPacket);

            UpdateUeState(ueId, UeState::Serving);
        }

        else if (cfRadioHeader.GetMessageType() == CfRadioHeader::TaskRequest)
        {
            auto ueId = cfRadioHeader.GetUeId();
            auto taskId = cfRadioHeader.GetTaskId();
            auto offloadServerId = cfRadioHeader.GetGnbId();

            NS_ASSERT(offloadServerId == m_serverId);

            MultiPacketHeader mpHeader;
            bool receiveCompleted = false;

            packet->RemoveHeader(mpHeader);
            receiveCompleted =
                m_multiPacketManager->AddAndCheckPacket(ueId,
                                                        taskId,
                                                        mpHeader.GetPacketId(),
                                                        mpHeader.GetTotalPacketNum());
            if (receiveCompleted)
            {
                NS_LOG_INFO("Remote server " << m_serverId << " Recv task request "
                                             << cfRadioHeader.GetTaskId() << " of UE "
                                             << cfRadioHeader.GetUeId());
                m_recvRequestTrace(ueId, taskId, Simulator::Now().GetTimeStep());
                m_addTaskTrace(ueId, taskId, Simulator::Now().GetTimeStep());

                UeTaskModel ueTask = m_cfranSystemInfo->GetUeInfo(ueId).m_taskModel;
                ueTask.m_taskId = cfRadioHeader.GetTaskId();
                m_cfUnit->LoadUeTask(ueId, ueTask);
            }
        }
        else
        {
            NS_FATAL_ERROR("Unkonwn CfRadioHeader");
        }
    }
}

void
RemoteCfApplication::RecvTaskResult(uint64_t ueId, UeTaskModel ueTask)
{
    NS_LOG_FUNCTION(this << ueId << ueTask.m_taskId);
    m_getResultTrace(ueId, ueTask.m_taskId, Simulator::Now().GetTimeStep());

    Ptr<Packet> resultPacket = Create<Packet>(500);

    CfRadioHeader echoHeader;
    echoHeader.SetUeId(ueId);
    echoHeader.SetMessageType(CfRadioHeader::TaskResult);
    echoHeader.SetGnbId(m_serverId);
    echoHeader.SetTaskId(ueTask.m_taskId);
    resultPacket->AddHeader(echoHeader);

    SendPacketToUe(ueId, resultPacket);

    m_sendResultTrace(ueId, ueTask.m_taskId, Simulator::Now().GetTimeStep());
}

void
RemoteCfApplication::StartApplication()
{
    NS_LOG_FUNCTION(this);

    if (!m_socket)
    {
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        m_socket = Socket::CreateSocket(GetNode(), tid);

        NS_LOG_DEBUG("Init  socket: " << m_port);
        InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_port);
        if (m_socket->Bind(local) == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        }
    }

    m_socket->SetRecvCallback(MakeCallback(&RemoteCfApplication::RecvFromUe, this));
    if (m_e2term != nullptr)
    {
        NS_LOG_DEBUG("E2sim start in cell " << m_serverId);
        Simulator::Schedule(Seconds(0), &E2Termination::Start, m_e2term);
    }
}

void
RemoteCfApplication::StopApplication()
{
    NS_LOG_FUNCTION(this);
}

void
RemoteCfApplication::BuildAndSendE2Report()
{
    NS_LOG_FUNCTION(this);

    NS_LOG_DEBUG(this << "m_e2term->GetSubscriptionState(): " << m_e2term->GetSubscriptionState());
    if (!m_e2term->GetSubscriptionState())
    {
        Simulator::Schedule(MilliSeconds(500), &RemoteCfApplication::BuildAndSendE2Report, this);
        return;
    }

    E2Termination::RicSubscriptionRequest_rval_s params = m_e2term->GetSubscriptionPara();
    std::string plmId = "111";
    std::string gnbId = std::to_string(this->GetServerId());
    uint16_t cellId = this->GetServerId();

    cJSON* msg = cJSON_CreateObject();
    cJSON_AddStringToObject(msg, "msgSource", "RemoteCfApp");
    cJSON_AddNumberToObject(msg, "ServerId", this->GetServerId());

    cJSON* ueMsgArray = cJSON_AddArrayToObject(msg, "UeMsgArray");

    for (auto it = m_ueState.begin(); it != m_ueState.end(); it++)
    {
        uint64_t ueId = it->first;

        CfranSystemInfo::UeInfo ueInfo = m_cfranSystemInfo->GetUeInfo(ueId);
        uint64_t assoCellId = ueInfo.m_mcUeNetDevice->GetMmWaveTargetEnb()->GetCellId();
        uint64_t compCellId = cellId;

        m_cfE2eCalaulator->BackupUeE2eResults(ueId, assoCellId, compCellId);

        std::vector<double> upWlDelay = m_cfE2eCalaulator->GetUplinkWirelessDelayStats(ueId);
        std::vector<double> upWdDelay = m_cfE2eCalaulator->GetUplinkWiredDelayStats(ueId);
        std::vector<double> queueDelay = m_cfE2eCalaulator->GetQueueDelayStats(ueId);
        std::vector<double> compDelay = m_cfE2eCalaulator->GetComputingDelayStats(ueId);
        std::vector<double> dnWdDelay = m_cfE2eCalaulator->GetDownlinkWiredDelayStats(ueId);
        std::vector<double> dnWlDelay = m_cfE2eCalaulator->GetDownlinkWirelessDelayStats(ueId);
        m_cfE2eCalaulator->ResetResultForUe(ueId);

        cJSON* ueMsg = cJSON_CreateObject();
        cJSON_AddNumberToObject(ueMsg, "Id", ueId);

        cJSON_AddNumberToObject(ueMsg, "upWlMea", upWlDelay[0] / 1e6);
        cJSON_AddNumberToObject(ueMsg, "upWlStd", upWlDelay[1] / 1e6);
        cJSON_AddNumberToObject(ueMsg, "upWlMin", upWlDelay[2] / 1e6);
        cJSON_AddNumberToObject(ueMsg, "upWlMax", upWlDelay[3] / 1e6);

        cJSON_AddNumberToObject(ueMsg, "upWdMea", upWdDelay[0] / 1e6);
        cJSON_AddNumberToObject(ueMsg, "upWdStd", upWdDelay[1] / 1e6);
        cJSON_AddNumberToObject(ueMsg, "upWdMin", upWdDelay[2] / 1e6);
        cJSON_AddNumberToObject(ueMsg, "upWdMax", upWdDelay[3] / 1e6);

        cJSON_AddNumberToObject(ueMsg, "queMea", queueDelay[0] / 1e6);
        cJSON_AddNumberToObject(ueMsg, "queStd", queueDelay[1] / 1e6);
        cJSON_AddNumberToObject(ueMsg, "queMin", queueDelay[2] / 1e6);
        cJSON_AddNumberToObject(ueMsg, "queMax", queueDelay[3] / 1e6);

        cJSON_AddNumberToObject(ueMsg, "compMea", compDelay[0] / 1e6);
        cJSON_AddNumberToObject(ueMsg, "compStd", compDelay[1] / 1e6);
        cJSON_AddNumberToObject(ueMsg, "compMin", compDelay[2] / 1e6);
        cJSON_AddNumberToObject(ueMsg, "compMax", compDelay[3] / 1e6);

        cJSON_AddNumberToObject(ueMsg, "dnWdMea", dnWdDelay[0] / 1e6);
        cJSON_AddNumberToObject(ueMsg, "dnWdStd", dnWdDelay[1] / 1e6);
        cJSON_AddNumberToObject(ueMsg, "dnWdMin", dnWdDelay[2] / 1e6);
        cJSON_AddNumberToObject(ueMsg, "dnWdMax", dnWdDelay[3] / 1e6);

        cJSON_AddNumberToObject(ueMsg, "dnWlMea", dnWlDelay[0] / 1e6);
        cJSON_AddNumberToObject(ueMsg, "dnWlStd", dnWlDelay[1] / 1e6);
        cJSON_AddNumberToObject(ueMsg, "dnWlMin", dnWlDelay[2] / 1e6);
        cJSON_AddNumberToObject(ueMsg, "dnWlMax", dnWlDelay[3] / 1e6);

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

    NS_LOG_DEBUG("Original len: " << strlen(reportString));
    // This is one way of getting the size of the output
    printf("Compressed size is: %lu\n", defstream.total_out);
    printf("Compressed size(wrong) is: %lu\n", strlen(b));
    printf("Compressed string is: %s\n", b);

    std::string base64String = base64_encode((const unsigned char*)b, defstream.total_out);
    NS_LOG_DEBUG("base64String: " << base64String);
    NS_LOG_DEBUG("base64String size: " << base64String.length());

    Ptr<KpmIndicationHeader> header = BuildRicIndicationHeader(plmId, gnbId, m_serverId);
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

    Simulator::ScheduleWithContext(GetNode()->GetId(),
                                   MilliSeconds(500),
                                   &RemoteCfApplication::BuildAndSendE2Report,
                                   this);
}

Ptr<KpmIndicationHeader>
RemoteCfApplication::BuildRicIndicationHeader(std::string plmId,
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

/**
 * KPM Subscription Request callback.
 * This function is triggered whenever a RIC Subscription Request for
 * the KPM RAN Function is received.
 *
 * \param pdu request message
 */
void
RemoteCfApplication::KpmSubscriptionCallback(E2AP_PDU_t* sub_req_pdu)
{
    NS_LOG_DEBUG("\nReceived RIC Subscription Request, cellId= " << m_serverId << "\n");

    E2Termination::RicSubscriptionRequest_rval_s params =
        m_e2term->ProcessRicSubscriptionRequest(sub_req_pdu);
    NS_LOG_DEBUG("requestorId " << +params.requestorId << ", instanceId " << +params.instanceId
                                << ", ranFuncionId " << +params.ranFuncionId << ", actionId "
                                << +params.actionId);
    BuildAndSendE2Report();
    // if (!m_isReportingEnabled)
    // {
    //     BuildAndSendReportMessage(params);
    //     m_isReportingEnabled = true;
    // }
}

} // namespace ns3