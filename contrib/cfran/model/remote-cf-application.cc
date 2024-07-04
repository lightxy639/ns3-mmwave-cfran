#include "remote-cf-application.h"

#include "cf-radio-header.h"
#include "multi-packet-header.h"
#include "zlib.h"

#include <ns3/base64.h>
#include <ns3/cJSON.h>
#include <ns3/log.h>
#include <encode_e2apv1.hpp>
#include <thread>

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

    // NS_LOG_INFO("Register E2SM");

    Ptr<KpmFunctionDescription> kpmFd = Create<KpmFunctionDescription>();
    e2term->RegisterKpmCallbackToE2Sm(
        200,
        kpmFd,
        std::bind(&RemoteCfApplication::KpmSubscriptionCallback, this, std::placeholders::_1));
}

void
RemoteCfApplication::SetClientFd(int clientFd)
{
    m_clientFd = clientFd;
    Simulator::ScheduleWithContext(GetNode()->GetId(),
                                   MilliSeconds(500),
                                   &RemoteCfApplication::BuildAndSendE2Report,
                                   this);
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
RemoteCfApplication::SendRefuseInformationToUe(uint64_t ueId)
{
    NS_LOG_FUNCTION(this);

    Ptr<Packet> packet = Create<Packet>(500);

    CfRadioHeader cfrHeader;
    cfrHeader.SetMessageType(CfRadioHeader::RefuseInform);
    cfrHeader.SetGnbId(m_serverId);
    cfrHeader.SetUeId(ueId);

    packet->AddHeader(cfrHeader);

    SendPacketToUe(ueId, packet);
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
            NS_LOG_INFO("RemoteServer " << m_serverId << " send Init Success to UE " << ueId);
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
                // NS_LOG_INFO("Remote server " << m_serverId << " Recv task request "
                //                              << cfRadioHeader.GetTaskId() << " of UE "
                //                              << cfRadioHeader.GetUeId());
                m_recvRequestTrace(ueId, taskId, Simulator::Now().GetTimeStep(), RecvRequest, None);
                m_addTaskTrace(ueId, taskId, Simulator::Now().GetTimeStep(), AddTask, None);

                UeTaskModel ueTask = m_cfranSystemInfo->GetUeInfo(ueId).m_taskModel;
                ueTask.m_taskId = cfRadioHeader.GetTaskId();
                m_cfUnit->LoadUeTask(ueId, ueTask);
            }
        }
        else if (cfRadioHeader.GetMessageType() == CfRadioHeader::TerminateCommand)
        {
            uint64_t ueId = cfRadioHeader.GetUeId();
            UpdateUeState(ueId, UeState::Over);
            m_cfUnit->DeleteUe(ueId);
            NS_LOG_INFO("RemoteServer " << m_serverId << " Recv command of UE " << ueId
                                        << " to terminate the service");

            SendUeEventMessage(ueId, CfranSystemInfo::UeRandomAction::Leave);
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
    // m_getResultTrace(ueId, ueTask.m_taskId, Simulator::Now().GetTimeStep(), GetResult, None);

    uint64_t resultDataSize = m_cfranSystemInfo->GetUeInfo(ueId).m_taskModel.m_downlinkSize;
    uint32_t packetNum = std::ceil((float)resultDataSize / m_defaultPacketSize);

    m_sendResultTrace(ueId, ueTask.m_taskId, Simulator::Now().GetTimeStep(), SendResult, None);

    for (uint32_t n = 1; n <= packetNum; n++)
    {
        MultiPacketHeader mpHeader;
        mpHeader.SetPacketId(n);
        mpHeader.SetTotalpacketNum(packetNum);

        CfRadioHeader cfrHeader;
        cfrHeader.SetUeId(ueId);
        cfrHeader.SetMessageType(CfRadioHeader::TaskResult);
        cfrHeader.SetGnbId(m_serverId);
        cfrHeader.SetTaskId(ueTask.m_taskId);

        Ptr<Packet> resultPacket = Create<Packet>(m_defaultPacketSize);

        resultPacket->AddHeader(mpHeader);
        resultPacket->AddHeader(cfrHeader);
        SendPacketToUe(ueId, resultPacket);
    }
}

void
RemoteCfApplication::StartApplication()
{
    NS_LOG_FUNCTION(this);

    if (!m_socket)
    {
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        m_socket = Socket::CreateSocket(GetNode(), tid);

        // NS_LOG_DEBUG("Init  socket: " << m_port);
        InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_port);
        if (m_socket->Bind(local) == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        }
    }

    m_socket->SetRecvCallback(MakeCallback(&RemoteCfApplication::RecvFromUe, this));
    if (m_e2term != nullptr)
    {
        NS_LOG_INFO("E2sim start in cell " << m_serverId);
        // Simulator::Schedule(Seconds(0), &E2Termination::Start, m_e2term);
        Simulator::ScheduleWithContext(GetNode()->GetId(),
                                       MicroSeconds(0),
                                       &E2Termination::Start,
                                       m_e2term);

        Ptr<RicControlFunctionDescription> ricCtrlFd = Create<RicControlFunctionDescription>();
        m_e2term->RegisterSmCallbackToE2Sm(
            300,
            ricCtrlFd,
            std::bind(&RemoteCfApplication::ControlMessageReceivedCallback,
                      this,
                      std::placeholders::_1));
    }
    else if (m_clientFd > 0)
    {
        std::thread recvPolicyTread(&RemoteCfApplication::RecvFromCustomSocket, this);
        recvPolicyTread.detach();
    }

    Simulator::Schedule(MilliSeconds(10), &RemoteCfApplication::ExecuteCommands, this);
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

    // NS_LOG_DEBUG(this << "m_e2term->GetSubscriptionState(): " <<
    // m_e2term->GetSubscriptionState());
    if (m_e2term != nullptr && !m_e2term->GetSubscriptionState())
    {
        Simulator::Schedule(MilliSeconds(100), &RemoteCfApplication::BuildAndSendE2Report, this);
        return;
    }

    // E2Termination::RicSubscriptionRequest_rval_s params = m_e2term->GetSubscriptionPara();
    std::string plmId = "111";
    std::string gnbId = std::to_string(this->GetServerId());
    uint16_t cellId = this->GetServerId();

    cJSON* msg = cJSON_CreateObject();
    cJSON_AddStringToObject(msg, "msgSource", "RemoteCfApp");
    cJSON_AddNumberToObject(msg, "serverId", this->GetServerId());
    cJSON_AddNumberToObject(msg, "updateTime", Simulator::Now().GetSeconds());
    cJSON_AddStringToObject(msg, "msgType", "KpmIndication");
    cJSON_AddNumberToObject(msg, "timeStamp", m_reportTimeStamp++);

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
        std::vector<double> e2eDelay = m_cfE2eCalaulator->GetE2eDelayStats(ueId);

        uint64_t taskCount = m_cfE2eCalaulator->GetNumberOfTimeData(ueId);
        uint64_t succCount = m_cfE2eCalaulator->GetNumberOfSuccTask(ueId);
        m_cfE2eCalaulator->ResetResultForUe(ueId);

        cJSON* ueStatsMsg = cJSON_CreateObject();
        cJSON_AddNumberToObject(ueStatsMsg, "Id", ueId);

        cJSON* uePos = cJSON_AddObjectToObject(ueStatsMsg, "pos");

        Vector pos = m_cfranSystemInfo->GetUeInfo(ueId)
                         .m_mcUeNetDevice->GetNode()
                         ->GetObject<MobilityModel>()
                         ->GetPosition();
        cJSON_AddNumberToObject(uePos, "x", pos.x);
        cJSON_AddNumberToObject(uePos, "y", pos.y);
        cJSON_AddNumberToObject(uePos, "z", pos.z);

        cJSON* latencyInfo = cJSON_AddObjectToObject(ueStatsMsg, "latency");
        cJSON_AddNumberToObject(latencyInfo, "taskCount", taskCount);
        cJSON_AddNumberToObject(latencyInfo, "succCount", succCount);
        cJSON_AddNumberToObject(latencyInfo, "upWlMea", upWlDelay[0] / 1e6);
        cJSON_AddNumberToObject(latencyInfo, "upWlStd", upWlDelay[1] / 1e6);
        cJSON_AddNumberToObject(latencyInfo, "upWlMin", upWlDelay[2] / 1e6);
        cJSON_AddNumberToObject(latencyInfo, "upWlMax", upWlDelay[3] / 1e6);

        cJSON_AddNumberToObject(latencyInfo, "upWdMea", upWdDelay[0] / 1e6);
        cJSON_AddNumberToObject(latencyInfo, "upWdStd", upWdDelay[1] / 1e6);
        cJSON_AddNumberToObject(latencyInfo, "upWdMin", upWdDelay[2] / 1e6);
        cJSON_AddNumberToObject(latencyInfo, "upWdMax", upWdDelay[3] / 1e6);

        cJSON_AddNumberToObject(latencyInfo, "queMea", queueDelay[0] / 1e6);
        cJSON_AddNumberToObject(latencyInfo, "queStd", queueDelay[1] / 1e6);
        cJSON_AddNumberToObject(latencyInfo, "queMin", queueDelay[2] / 1e6);
        cJSON_AddNumberToObject(latencyInfo, "queMax", queueDelay[3] / 1e6);

        cJSON_AddNumberToObject(latencyInfo, "compMea", compDelay[0] / 1e6);
        cJSON_AddNumberToObject(latencyInfo, "compStd", compDelay[1] / 1e6);
        cJSON_AddNumberToObject(latencyInfo, "compMin", compDelay[2] / 1e6);
        cJSON_AddNumberToObject(latencyInfo, "compMax", compDelay[3] / 1e6);

        cJSON_AddNumberToObject(latencyInfo, "dnWdMea", dnWdDelay[0] / 1e6);
        cJSON_AddNumberToObject(latencyInfo, "dnWdStd", dnWdDelay[1] / 1e6);
        cJSON_AddNumberToObject(latencyInfo, "dnWdMin", dnWdDelay[2] / 1e6);
        cJSON_AddNumberToObject(latencyInfo, "dnWdMax", dnWdDelay[3] / 1e6);

        cJSON_AddNumberToObject(latencyInfo, "dnWlMea", dnWlDelay[0] / 1e6);
        cJSON_AddNumberToObject(latencyInfo, "dnWlStd", dnWlDelay[1] / 1e6);
        cJSON_AddNumberToObject(latencyInfo, "dnWlMin", dnWlDelay[2] / 1e6);
        cJSON_AddNumberToObject(latencyInfo, "dnWlMax", dnWlDelay[3] / 1e6);

        cJSON_AddNumberToObject(latencyInfo, "e2eMea", e2eDelay[0] / 1e6);
        cJSON_AddNumberToObject(latencyInfo, "e2eStd", e2eDelay[1] / 1e6);
        cJSON_AddNumberToObject(latencyInfo, "e2eMin", e2eDelay[2] / 1e6);
        cJSON_AddNumberToObject(latencyInfo, "e2eMax", e2eDelay[3] / 1e6);
        
        cJSON_AddItemToArray(ueMsgArray, ueStatsMsg);
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
        Ptr<KpmIndicationHeader> header = BuildRicIndicationHeader(plmId, gnbId, m_serverId);
        E2AP_PDU* pdu_du_ue = new E2AP_PDU;
        auto kpmPrams = m_e2term->GetSubscriptionPara();
        // NS_LOG_DEBUG("kpmPrams.ranFuncionId: " << kpmPrams.ranFuncionId);
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

    // NS_LOG_INFO("RemoteCfApplication " << m_serverId << " send indication message");
    Simulator::ScheduleWithContext(GetNode()->GetId(),
                                   Seconds(m_e2ReportPeriod),
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
    // NS_LOG_DEBUG("NR plmid " << plmId << " gnbId " << gnbId << " nrCellId " << nrCellId);
    // NS_LOG_DEBUG("Timestamp " << timestamp);
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

void
RemoteCfApplication::ControlMessageReceivedCallback(E2AP_PDU_t* pdu)
{
    // NS_LOG_DEBUG ("Control Message Received, cellId is " << m_gnbNetDev->GetCellId ());
    std::cout << "Control Message Received, serverId is " << this->GetServerId() << std::endl;
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

void
RemoteCfApplication::SendUeEventMessage(uint64_t ueId, CfranSystemInfo::UeRandomAction action)
{
    NS_LOG_FUNCTION(this);

    cJSON* msg = cJSON_CreateObject();
    cJSON_AddStringToObject(msg, "msgSource", "RemoteCfApp");
    cJSON_AddNumberToObject(msg, "serverId", m_serverId);

    cJSON_AddStringToObject(msg, "msgType", "Event");

    cJSON* ueEvent = cJSON_AddObjectToObject(msg, "ueEvent");

    cJSON_AddNumberToObject(ueEvent, "ueId", ueId);

    if (action == CfranSystemInfo::Arrive)
    {
        cJSON_AddStringToObject(ueEvent, "action", "Arrive");
    }
    else if (action == CfranSystemInfo::Leave)
    {
        cJSON_AddStringToObject(ueEvent, "action", "Leave");
    }

    CfranSystemInfo::UeInfo ueInfo = m_cfranSystemInfo->GetUeInfo(ueId);
    cJSON_AddNumberToObject(ueEvent, "cfLoad", ueInfo.m_taskModel.m_cfLoad);
    cJSON_AddNumberToObject(ueEvent, "periodity", ueInfo.m_taskPeriodity);
    cJSON_AddNumberToObject(ueEvent, "deadline", ueInfo.m_taskModel.m_deadline);
    cJSON_AddNumberToObject(ueEvent, "uplinkSize", ueInfo.m_taskModel.m_uplinkSize);
    cJSON_AddNumberToObject(ueEvent, "downlinkSize", ueInfo.m_taskModel.m_downlinkSize);

    std::string cJsonPrint = cJSON_PrintUnformatted(msg);
    const char* reportString = cJsonPrint.c_str();

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

    std::string base64String = base64_encode((const unsigned char*)b, defstream.total_out);

    if (m_e2term != nullptr)
    {
        std::string plmId = "111";
        std::string gnbId = std::to_string(m_serverId);
        uint16_t cellId = m_serverId;

        Ptr<KpmIndicationHeader> header = BuildRicIndicationHeader(plmId, gnbId, cellId);
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

    NS_LOG_INFO("RemoteServer " << m_serverId << " send event message of UE " << ueId);
}

void
RemoteCfApplication::RecvFromCustomSocket()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("Recv thread start");

    char buffer[8196] = {0};

    while (true)
    {
        memset(buffer, 0, sizeof(buffer));
        if (recv(m_clientFd, buffer, sizeof(buffer) - 1, 0) < 0)
        {
            break;
        }

        cJSON* json = cJSON_Parse(buffer);
        if (json == nullptr)
        {
            NS_LOG_ERROR("Parsing json failed");
        }
        else
        {
            NS_LOG_INFO("GnbCfApp " << m_serverId << " Recv Policy message: " << buffer);
            PrasePolicyMessage(json);
        }
    }
}

void
RemoteCfApplication::PrasePolicyMessage(cJSON* json)
{
    NS_LOG_DEBUG(this);

    cJSON* uePolicy = nullptr;
    cJSON_ArrayForEach(uePolicy, json)
    {
        cJSON* ueId = cJSON_GetObjectItemCaseSensitive(uePolicy, "ueId");
        cJSON* action = cJSON_GetObjectItemCaseSensitive(uePolicy, "action");

        if (cJSON_IsNumber(ueId) && cJSON_IsString(action))
        {
            Policy uePolicy;
            uePolicy.m_ueId = ueId->valueint;
            if (std::string(action->valuestring) == "StartService")
            {
                uePolicy.m_action = Action::StartService;
            }
            else if (std::string(action->valuestring) == "StopService")
            {
                uePolicy.m_action = Action::StopService;
            }
            else if (std::string(action->valuestring) == "RefuseService")
            {
                uePolicy.m_action = Action::RefuseService;
            }
            else
            {
                NS_FATAL_ERROR("Invalid action");
            }

            m_policy.push(uePolicy);
        }
        else
        {
            NS_FATAL_ERROR("Invalid json");
        }
    }
}

void
RemoteCfApplication::ExecuteCommands()
{
    while (!m_policy.empty())
    {
        Policy uePolicy = m_policy.front();
        m_policy.pop();

        uint64_t ueId = uePolicy.m_ueId;
        Action action = uePolicy.m_action;

        if (action == Action::StartService)
        {
            NS_LOG_INFO("RemoteServer " << m_serverId << " start service for UE " << ueId);
            UpdateUeState(ueId, UeState::Initializing);

            m_cfUnit->AddNewUe(ueId);

            Ptr<Packet> resultPacket = Create<Packet>(500);
            CfRadioHeader echoHeader;
            echoHeader.SetMessageType(CfRadioHeader::InitSuccess);
            echoHeader.SetGnbId(m_serverId);
            resultPacket->AddHeader(echoHeader);
            SendPacketToUe(ueId, resultPacket);

            UpdateUeState(ueId, UeState::Serving);

            NS_LOG_INFO("RemoteServer " << m_serverId << " send Init Success to UE " << ueId);
        }
        else if (action == Action::StopService)
        {
            NS_LOG_INFO("RemoteServer " << m_serverId << " stop service for UE " << ueId);
            UpdateUeState(ueId, UeState::Over);
            m_cfUnit->DeleteUe(ueId);
        }
        else if (action == Action::RefuseService)
        {
            NS_LOG_INFO("RemoteServer " << m_serverId << " send refuse information to UE " << ueId);

            if (m_ueState.find(ueId) != m_ueState.end())
            {
                UpdateUeState(ueId, UeState::Over);
                m_cfUnit->DeleteUe(ueId);
            }
            Ptr<Packet> refusePacket = Create<Packet>(500);
            CfRadioHeader cfrHeader;
            cfrHeader.SetMessageType(CfRadioHeader::RefuseInform);
            cfrHeader.SetGnbId(m_serverId);
            refusePacket->AddHeader(cfrHeader);

            SendPacketToUe(ueId, refusePacket);
        }
    }

    Simulator::Schedule(MilliSeconds(5), &RemoteCfApplication::ExecuteCommands, this);
}
} // namespace ns3