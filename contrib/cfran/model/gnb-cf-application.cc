#include "gnb-cf-application.h"

// #include "encode_e2apv1.hpp"
#include "zlib.h"

#include <ns3/base64.h>
#include <ns3/cf-radio-header.h>
#include <ns3/cf-x2-header.h>
#include <ns3/epc-x2.h>
#include <ns3/ipv4-header.h>
#include <ns3/ipv4-l3-protocol.h>
#include <ns3/log.h>
#include <ns3/lte-pdcp-header.h>
#include <ns3/lte-pdcp-tag.h>
#include <ns3/packet-socket-address.h>
#include <ns3/task-request-header.h>

#include <encode_e2apv1.hpp>
#include <thread>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("GnbCfApplication");

CfX2IfaceInfo::CfX2IfaceInfo(Ipv4Address remoteIpAddr, Ptr<Socket> localSocket)
{
    m_remoteIpAddr = remoteIpAddr;
    m_localSocket = localSocket;
}

CfX2IfaceInfo::~CfX2IfaceInfo(void)
{
    m_localSocket = 0;
}

CfX2IfaceInfo&
CfX2IfaceInfo::operator=(const CfX2IfaceInfo& value)
{
    NS_LOG_FUNCTION(this);
    m_remoteIpAddr = value.m_remoteIpAddr;
    m_localSocket = value.m_localSocket;
    return *this;
}

///////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(GnbCfApplication);

TypeId
GnbCfApplication::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::GnbCfApplication")
            .SetParent<CfApplication>()
            .AddConstructor<GnbCfApplication>()
            .AddAttribute(
                "PolicyMode",
                "Process UEs' init requests locally or report and wait for a decision",
                EnumValue(GnbCfApplication::E2),
                MakeEnumAccessor(&GnbCfApplication::m_policyMode),
                MakeEnumChecker(GnbCfApplication::Local, "Local", GnbCfApplication::E2, "E2"))
            // .AddTraceSource("ForwardRequest",
            //                 "Forward UE task request of UE",
            //                 MakeTraceSourceAccessor(&GnbCfApplication::m_forwardRequestTrace),
            //                 "ns3::UlRequestForwardTx::TracedCallback")
            .AddTraceSource(
                "RecvRequestToBeForwarded",
                "Recv UE task request to be forwarded",
                MakeTraceSourceAccessor(&GnbCfApplication::m_recvRequestToBeForwardedTrace),
                "ns3::UlRequestToBeForwardRx::TracedCallback")
            // .AddTraceSource("RecvForwardedRequest",
            //                 "Recv forwarded UE task request of UE",
            //                 MakeTraceSourceAccessor(&GnbCfApplication::m_recvForwardedRequestTrace),
            //                 "ns3::UlRequestForwardedRx::TracedCallback")
            // .AddTraceSource("ForwardResult",
            //                 "Forward task result to corresponding gNB",
            //                 MakeTraceSourceAccessor(&GnbCfApplication::m_forwardResultTrace),
            //                 "ns3::ResultWiredDownlink::TracedCallback")
            .AddTraceSource("RecvForwardedResult",
                            "Recv forwarded task result",
                            MakeTraceSourceAccessor(&GnbCfApplication::m_recvForwardedResultTrace),
                            "ns3::ResultWiredDownlinkRx::TracedCallback");

    return tid;
}

GnbCfApplication::GnbCfApplication()
    : m_cfX2Port(4275),
      m_appSize(50000000)
{
    NS_LOG_FUNCTION(this);
    m_requestManager = CreateObject<MultiPacketManager>();
    m_resultManager = CreateObject<MultiPacketManager>();
    m_appDataManager = CreateObject<MultiPacketManager>();
}

GnbCfApplication::~GnbCfApplication()
{
    NS_LOG_FUNCTION(this);
}

void
GnbCfApplication::SetMmWaveEnbNetDevice(Ptr<mmwave::MmWaveEnbNetDevice> mmwaveEnbNetDev)
{
    m_mmWaveEnbNetDevice = mmwaveEnbNetDev;
}

Ptr<mmwave::MmWaveEnbNetDevice>
GnbCfApplication::GetMmWaveEnbNetDevice()
{
    return m_mmWaveEnbNetDevice;
}

void
GnbCfApplication::SetClientFd(int clientFd)
{
    m_clientFd = clientFd;

    Simulator::ScheduleWithContext(GetNode()->GetId(),
                                   MilliSeconds(100),
                                   &GnbCfApplication::BuildAndSendE2Report,
                                   this);
}

void
GnbCfApplication::RecvTaskRequest()
{
    NS_LOG_FUNCTION(this);
}

void
GnbCfApplication::SendPacketToUe(uint64_t ueId, Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(this << ueId);
    CfranSystemInfo::UeInfo ueInfo = m_cfranSystemInfo->GetUeInfo(ueId);
    Ptr<ns3::mmwave::McUeNetDevice> mcUeNetDev = ueInfo.m_mcUeNetDevice;

    Ptr<ns3::mmwave::MmWaveEnbNetDevice> ueMmWaveEnbNetDev = mcUeNetDev->GetMmWaveTargetEnb();
    NS_ASSERT(ueMmWaveEnbNetDev != nullptr);

    uint16_t lteRnti = mcUeNetDev->GetLteRrc()->GetRnti();
    Ptr<LteEnbNetDevice> lteEnbNetDev = mcUeNetDev->GetLteTargetEnb();
    auto drbMap = lteEnbNetDev->GetRrc()->GetUeManager(lteRnti)->GetDrbMap();
    uint32_t gtpTeid = (drbMap.begin()->second->m_gtpTeid);

    EpcX2RlcUser* epcX2RlcUser =
        ueMmWaveEnbNetDev->GetNode()->GetObject<EpcX2>()->GetX2RlcUserMap().find(gtpTeid)->second;

    PdcpTag pdcpTag(Simulator::Now());
    packet->AddByteTag(pdcpTag);
    // Adding pdcptag is necessary since mc-ue-pdcp will remove 2-bit pdcpheader
    LtePdcpHeader pdcpHeader;
    packet->AddHeader(pdcpHeader);

    NS_ASSERT(epcX2RlcUser != nullptr);
    if (epcX2RlcUser != nullptr)
    {
        EpcX2SapUser::UeDataParams params;

        params.gtpTeid = gtpTeid;
        params.ueData = packet;
        PdcpTag pdcpTag(Simulator::Now());

        params.ueData->AddByteTag(pdcpTag);
        epcX2RlcUser->SendMcPdcpSdu(params);
    }
}

void
GnbCfApplication::SendPacketToOtherGnb(uint64_t gnbId, Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(this);
    if (m_cfX2InterfaceSockets[gnbId]->m_localSocket->Send(packet) >= 0)
    {
        // NS_LOG_DEBUG("Cell " << m_mmWaveEnbNetDevice->GetCellId() << " send paclet to gnb " <<
        // gnbId
        //                      << " ip " << m_cfX2InterfaceSockets[gnbId]->m_remoteIpAddr);
    }
    else
    {
        NS_FATAL_ERROR("Send error");
    }
}

void
GnbCfApplication::SendInitSuccessToUserFromGnb(uint64_t id)
{
    NS_LOG_FUNCTION(this);

    Ptr<Packet> resultPacket = Create<Packet>(500);

    CfRadioHeader echoHeader;
    echoHeader.SetMessageType(CfRadioHeader::InitSuccess);
    echoHeader.SetGnbId(m_mmWaveEnbNetDevice->GetCellId());

    resultPacket->AddHeader(echoHeader);

    SendPacketToUe(id, resultPacket);

    NS_LOG_INFO("Gnb " << m_mmWaveEnbNetDevice->GetCellId() << " send Init Success to UE " << id);
}

void
GnbCfApplication::SendInitSuccessToConnectedGnb(uint64_t ueId)
{
    NS_LOG_FUNCTION(this);

    uint64_t ueConnectedGnbId =
        m_cfranSystemInfo->GetUeInfo(ueId).m_mcUeNetDevice->GetMmWaveTargetEnb()->GetCellId();
    Ptr<Packet> resultPacket = Create<Packet>(500);

    CfX2Header cfX2Header;
    cfX2Header.SetMessageType(CfX2Header::InitSuccess);
    cfX2Header.SetSourceGnbId(m_mmWaveEnbNetDevice->GetCellId());
    cfX2Header.SetTargetGnbId(ueConnectedGnbId);
    cfX2Header.SetUeId(ueId);

    resultPacket->AddHeader(cfX2Header);

    SendPacketToOtherGnb(ueConnectedGnbId, resultPacket);
    NS_LOG_INFO("Gnb " << m_mmWaveEnbNetDevice->GetCellId() << " send Init Success for UE " << ueId
                       << " to Gnb " << ueConnectedGnbId);
}

void
GnbCfApplication::SendTaskResultToUserFromRemote(uint64_t id, Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(this);
}

void
GnbCfApplication::StartApplication()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("cell " << m_mmWaveEnbNetDevice->GetCellId() << " start application");

    if (!m_socket)
    {
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        m_socket = Socket::CreateSocket(GetNode(), tid);
        InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_port);
        if (m_socket->Bind(local) == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        }
    }
    // m_socket->SetRecvCallback(MakeCallback(&GnbCfApplication::HandleRead, this));
    m_socket->SetRecvCallback(MakeCallback(&GnbCfApplication::RecvFromUe, this));

    InitX2Info();

    if (m_e2term)
    {
        Simulator::Schedule(MilliSeconds(100), &GnbCfApplication::BuildAndSendE2Report, this);
        Ptr<RicControlFunctionDescription> ricCtrlFd = Create<RicControlFunctionDescription>();
        m_e2term->RegisterSmCallbackToE2Sm(
            300,
            ricCtrlFd,
            std::bind(&GnbCfApplication::ControlMessageReceivedCallback,
                      this,
                      std::placeholders::_1));
    }

    else if (m_clientFd > 0)
    {
        std::thread recvPolicyTread(&GnbCfApplication::RecvFromCustomSocket, this);
        recvPolicyTread.detach();
    }
    Simulator::Schedule(MilliSeconds(10), &GnbCfApplication::ExecuteCommands, this);
    // else if (m_clientFd > 0)
    // {

    // }
}

void
GnbCfApplication::StopApplication()
{
    NS_LOG_FUNCTION(this);
}

// void
// GnbCfApplication::LoadTaskToCfUnit(uint64_t id, UeTaskModel ueTask)
// {
//     NS_LOG_FUNCTION(this);
// }

void
GnbCfApplication::RecvTaskResult(uint64_t ueId, UeTaskModel ueTask)
{
    NS_LOG_FUNCTION(this << ueId << ueTask.m_taskId);

    // m_getResultTrace(ueId, ueTask.m_taskId, Simulator::Now().GetTimeStep(), GetResult, None);

    auto ueConnectedGnbId =
        m_cfranSystemInfo->GetUeInfo(ueId).m_mcUeNetDevice->GetMmWaveTargetEnb()->GetCellId();
    auto appGnbId = m_mmWaveEnbNetDevice->GetCellId();

    if (ueConnectedGnbId == appGnbId)
    {
        // SendTaskResultToUserFromGnb(ueId);
        m_sendResultTrace(ueId, ueTask.m_taskId, Simulator::Now().GetTimeStep(), SendResult, None);

        uint64_t resultDataSize = m_cfranSystemInfo->GetUeInfo(ueId).m_taskModel.m_downlinkSize;
        uint32_t packetNum = std::ceil((float)resultDataSize / m_defaultPacketSize);

        for (uint32_t n = 1; n <= packetNum; n++)
        {
            MultiPacketHeader mpHeader;
            mpHeader.SetPacketId(n);
            mpHeader.SetTotalpacketNum(packetNum);

            CfRadioHeader cfrHeader;
            cfrHeader.SetUeId(ueId);
            cfrHeader.SetMessageType(CfRadioHeader::TaskResult);
            cfrHeader.SetGnbId(m_mmWaveEnbNetDevice->GetCellId());
            cfrHeader.SetTaskId(ueTask.m_taskId);

            Ptr<Packet> p = Create<Packet>(m_defaultPacketSize);
            p->AddHeader(mpHeader);
            p->AddHeader(cfrHeader);
            SendPacketToUe(ueId, p);
        }

        // Ptr<Packet> resultPacket = Create<Packet>(500);

        // CfRadioHeader echoHeader;
        // echoHeader.SetUeId(ueId);
        // echoHeader.SetMessageType(CfRadioHeader::TaskResult);
        // echoHeader.SetGnbId(m_mmWaveEnbNetDevice->GetCellId());
        // echoHeader.SetTaskId(ueTask.m_taskId);
        // resultPacket->AddHeader(echoHeader);

        // m_downlinkTrace(ueId, ueTask.m_taskId, Simulator::Now().GetTimeStep());
    }
    else
    {
        // m_recvForwardedResultTrace(ueId, ueTask.m_taskId, Simulator::Now().GetTimeStep(),
        // RecvForwardedResult, None);

        m_sendResultTrace(ueId, ueTask.m_taskId, Simulator::Now().GetTimeStep(), SendResult, None);
        uint64_t resultDataSize = m_cfranSystemInfo->GetUeInfo(ueId).m_taskModel.m_downlinkSize;
        uint32_t packetNum = std::ceil((float)resultDataSize / m_defaultPacketSize);

        for (uint32_t n = 1; n <= packetNum; n++)
        {
            MultiPacketHeader mpHeader;
            mpHeader.SetPacketId(n);
            mpHeader.SetTotalpacketNum(packetNum);

            CfX2Header cfX2Header;
            cfX2Header.SetMessageType(CfX2Header::TaskResult);
            cfX2Header.SetSourceGnbId(m_mmWaveEnbNetDevice->GetCellId());
            cfX2Header.SetTargetGnbId(ueConnectedGnbId);
            cfX2Header.SetUeId(ueId);
            cfX2Header.SetTaskId(ueTask.m_taskId);

            Ptr<Packet> p = Create<Packet>(m_defaultPacketSize);
            p->AddHeader(mpHeader);
            p->AddHeader(cfX2Header);

            SendPacketToOtherGnb(ueConnectedGnbId, p);
        }
    }

    NS_LOG_INFO("Gnb " << m_mmWaveEnbNetDevice->GetCellId() << " recv task result of (UE,Task) "
                       << ueId << " " << ueTask.m_taskId);
}

void
GnbCfApplication::HandleRead(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_UNCOND("GnbCfApplication Receive IPv4 packet");
    Ptr<Packet> packet;
    Address from;
    Address localAddress;

    while ((packet = socket->RecvFrom(from)))
    {
        socket->GetSockName(localAddress);

        if (InetSocketAddress::IsMatchingType(from))
        {
            NS_LOG_INFO("At time " << Simulator::Now().As(Time::S) << " server received "
                                   << packet->GetSize() << " bytes from "
                                   << InetSocketAddress::ConvertFrom(from).GetIpv4() << " port "
                                   << InetSocketAddress::ConvertFrom(from).GetPort());
        }
        else if (Inet6SocketAddress::IsMatchingType(from))
        {
            NS_LOG_INFO("At time " << Simulator::Now().As(Time::S) << " server received "
                                   << packet->GetSize() << " bytes from "
                                   << Inet6SocketAddress::ConvertFrom(from).GetIpv6() << " port "
                                   << Inet6SocketAddress::ConvertFrom(from).GetPort());
        }

        TaskRequestHeader taskReqHeader;
        packet->RemoveHeader(taskReqHeader);
        NS_LOG_DEBUG("Recv task req header of ue " << taskReqHeader.GetUeId());
        NS_LOG_LOGIC("Echoing packet");
        Ptr<Packet> resultPacket = Create<Packet>(1200);
    }
}

void
GnbCfApplication::RecvFromUe(Ptr<Socket> socket)
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
            NS_LOG_INFO("Gnb " << m_mmWaveEnbNetDevice->GetCellId() << "Recv init request of UE "
                               << cfRadioHeader.GetUeId());

            if (m_policyMode == Local)
            {
                // NS_LOG_INFO("Process UEs' init requests locally");
                UpdateUeState(cfRadioHeader.GetUeId(), UeState::Initializing);
                m_cfUnit->AddNewUe(cfRadioHeader.GetUeId());
                SendInitSuccessToUserFromGnb(cfRadioHeader.GetUeId());
                UpdateUeState(cfRadioHeader.GetUeId(), UeState::Serving);
            }
            else if (m_policyMode == E2)
            {
                // NS_LOG_INFO("Report to RIC and wait for command.");
                // SendNewUeReport(cfRadioHeader.GetUeId());
                SendUeEventMessage(cfRadioHeader.GetUeId(),
                                   CfranSystemInfo::UeRandomAction::Arrive);
                // AssignUe(cfRadioHeader.GetUeId(), 5);
            }
        }
        else if (cfRadioHeader.GetMessageType() == CfRadioHeader::TaskRequest)
        {
            auto ueId = cfRadioHeader.GetUeId();
            auto taskId = cfRadioHeader.GetTaskId();
            auto hereGnbId = m_mmWaveEnbNetDevice->GetCellId();
            auto offloadGnbId = cfRadioHeader.GetGnbId();

            if (hereGnbId == offloadGnbId)
            {
                MultiPacketHeader mpHeader;
                bool receiveCompleted = false;

                packet->RemoveHeader(mpHeader);
                receiveCompleted =
                    m_requestManager->AddAndCheckPacket(ueId,
                                                        taskId,
                                                        mpHeader.GetPacketId(),
                                                        mpHeader.GetTotalPacketNum());
                if (receiveCompleted)
                {
                    NS_LOG_INFO("Gnb " << m_mmWaveEnbNetDevice->GetCellId()
                                       << " Recv task request "
                                       << cfRadioHeader.GetTaskId() << " of UE "
                                       << cfRadioHeader.GetUeId());
                    m_recvRequestTrace(ueId,
                                       taskId,
                                       Simulator::Now().GetTimeStep(),
                                       RecvRequest,
                                       None);
                    m_addTaskTrace(ueId, taskId, Simulator::Now().GetTimeStep(), AddTask, None);

                    UeTaskModel ueTask = m_cfranSystemInfo->GetUeInfo(ueId).m_taskModel;
                    ueTask.m_taskId = cfRadioHeader.GetTaskId();
                    m_cfUnit->LoadUeTask(ueId, ueTask);
                }
            }
            else
            {
                NS_LOG_INFO("Gnb " << hereGnbId << " Send the request of UE "
                                   << cfRadioHeader.GetUeId() << " to other gNB.");

                MultiPacketHeader mpHd;
                packet->PeekHeader(mpHd);
                bool receiveCompleted =
                    m_requestManager->AddAndCheckPacket(ueId,
                                                        taskId,
                                                        mpHd.GetPacketId(),
                                                        mpHd.GetTotalPacketNum());
                if (receiveCompleted)
                {
                    // m_recvRequestTrace(ueId, taskId, Simulator::Now().GetTimeStep(), RecvRequest,
                    // None);
                    m_recvRequestToBeForwardedTrace(ueId,
                                                    taskId,
                                                    Simulator::Now().GetTimeStep(),
                                                    RecvRequestToBeForwarded,
                                                    None);
                    // m_forwardRequestTrace(ueId, taskId, Simulator::Now().GetTimeStep());
                }

                CfX2Header x2Hd;
                x2Hd.SetMessageType(CfX2Header::TaskRequest);
                x2Hd.SetSourceGnbId(hereGnbId);
                x2Hd.SetTargetGnbId(offloadGnbId);
                x2Hd.SetUeId(ueId);
                x2Hd.SetTaskId(cfRadioHeader.GetTaskId());
                packet->AddHeader(x2Hd);
                SendPacketToOtherGnb(offloadGnbId, packet);
            }
        }
        else if (cfRadioHeader.GetMessageType() == CfRadioHeader::TerminateCommand)
        {
            uint64_t hereGnbId = m_mmWaveEnbNetDevice->GetCellId();
            uint64_t offloadGnbId = cfRadioHeader.GetGnbId();
            uint64_t ueId = cfRadioHeader.GetUeId();

            SendUeEventMessage(ueId, CfranSystemInfo::Leave);

            if (hereGnbId == offloadGnbId)
            {
                UpdateUeState(ueId, UeState::Over);
                m_cfUnit->DeleteUe(ueId);
                NS_LOG_INFO("UE " << ueId << " terminate the service");
            }
            else
            {
                NS_LOG_INFO("Gnb " << m_mmWaveEnbNetDevice->GetCellId()
                                   << " Recv terminate command to Gnb " << offloadGnbId);
                CfX2Header cfX2Header;
                cfX2Header.SetMessageType(CfX2Header::TerminateCommand);
                cfX2Header.SetUeId(ueId);
                cfX2Header.SetSourceGnbId(hereGnbId);
                cfX2Header.SetTargetGnbId(offloadGnbId);
                packet->AddHeader(cfX2Header);

                SendPacketToOtherGnb(offloadGnbId, packet);
            }
        }
        else
        {
            NS_FATAL_ERROR("Something wrong");
        }
    }
}

void
GnbCfApplication::InitX2Info()
{
    NS_LOG_FUNCTION(this);

    Ptr<EpcX2> epcX2 = m_node->GetObject<EpcX2>();
    NS_ASSERT(epcX2 != nullptr);

    auto x2IfaceInfoMap = epcX2->GetX2IfaceInfoMap();
    for (auto it = x2IfaceInfoMap.begin(); it != x2IfaceInfoMap.end(); it++)
    {
        uint16_t remoteCellId = it->first;
        Ptr<X2IfaceInfo> x2IfaceInfo = it->second;

        Ipv4Address remoteIpAddr = x2IfaceInfo->m_remoteIpAddr;
        Ptr<Socket> localUSocket = x2IfaceInfo->m_localUserPlaneSocket;

        Address tempAddr;
        localUSocket->GetSockName(tempAddr);
        Ipv4Address localAddr = InetSocketAddress::ConvertFrom(tempAddr).GetIpv4();

        NS_LOG_DEBUG("IP of the cell: " << localAddr);
        NS_LOG_DEBUG("Get IP adderess of cell " << remoteCellId << ": " << remoteIpAddr);

        Ptr<Socket> localCfSocket =
            Socket::CreateSocket(m_node, TypeId::LookupByName("ns3::UdpSocketFactory"));
        localCfSocket->Bind(InetSocketAddress(localAddr, m_cfX2Port));
        localCfSocket->Connect(InetSocketAddress(remoteIpAddr, m_cfX2Port));
        NS_ASSERT_MSG(m_cfX2InterfaceSockets.find(remoteCellId) == m_cfX2InterfaceSockets.end(),
                      "Mapping for remoteCellId = " << remoteCellId << " is already known");

        localCfSocket->SetRecvCallback(MakeCallback(&GnbCfApplication::RecvFromOtherGnb, this));
        m_cfX2InterfaceSockets[remoteCellId] = Create<CfX2IfaceInfo>(remoteIpAddr, localCfSocket);
    }
}

void
GnbCfApplication::RecvFromOtherGnb(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this);
    Ptr<Packet> packet;
    Address from;
    Address localAddress;

    while (packet = socket->RecvFrom(from))
    {
        socket->GetSockName(localAddress);

        CfX2Header cfX2Header;
        packet->RemoveHeader(cfX2Header);

        if (cfX2Header.GetMessageType() == CfX2Header::InitRequest)
        {
            NS_LOG_INFO("Gnb " << m_mmWaveEnbNetDevice->GetCellId() << "Recv init request of UE "
                               << cfX2Header.GetUeId() << " from gnb "
                               << cfX2Header.GetSourceGnbId());
            if (m_ueState.find(cfX2Header.GetUeId()) != m_ueState.end())
            {
                NS_LOG_UNCOND("Repeated initialization with redundant instructions");
            }

            else
            {
                UpdateUeState(cfX2Header.GetUeId(), UeState::Initializing);
                m_cfUnit->AddNewUe(cfX2Header.GetUeId());
                SendInitSuccessToConnectedGnb(cfX2Header.GetUeId());
                UpdateUeState(cfX2Header.GetUeId(), UeState::Serving);
            }
        }
        else if (cfX2Header.GetMessageType() == CfX2Header::TaskRequest)
        {
            auto ueId = cfX2Header.GetUeId();
            auto taskId = cfX2Header.GetTaskId();

            MultiPacketHeader mpHeader;
            bool receiveCompleted = false;

            packet->RemoveHeader(mpHeader);
            receiveCompleted = m_requestManager->AddAndCheckPacket(ueId,
                                                                   taskId,
                                                                   mpHeader.GetPacketId(),
                                                                   mpHeader.GetTotalPacketNum());
            if (receiveCompleted)
            {
                // NS_LOG_INFO("Gnb " << m_mmWaveEnbNetDevice->GetCellId()
                //                    << " Recv task request of UE " << cfX2Header.GetUeId()
                //                    << " from gnb " << cfX2Header.GetSourceGnbId());
                // m_recvForwardedRequestTrace(ueId, taskId, Simulator::Now().GetTimeStep(), );
                m_recvRequestTrace(ueId, taskId, Simulator::Now().GetTimeStep(), RecvRequest, None);
                m_addTaskTrace(ueId, taskId, Simulator::Now().GetTimeStep(), AddTask, None);

                UeTaskModel ueTask = m_cfranSystemInfo->GetUeInfo(ueId).m_taskModel;
                ueTask.m_taskId = taskId;
                m_cfUnit->LoadUeTask(ueId, ueTask);
            }
        }
        else if (cfX2Header.GetMessageType() == CfX2Header::InitSuccess)
        {
            NS_LOG_INFO("Gnb " << m_mmWaveEnbNetDevice->GetCellId()
                               << " Recv forwarded InitSuccess of UE " << cfX2Header.GetUeId()
                               << " from gnb " << cfX2Header.GetSourceGnbId());

            auto ueId = cfX2Header.GetUeId();
            auto offloadGnbId = cfX2Header.GetSourceGnbId();

            CfRadioHeader cfRadioHeader;
            cfRadioHeader.SetMessageType(CfRadioHeader::InitSuccess);
            cfRadioHeader.SetUeId(ueId);
            cfRadioHeader.SetGnbId(offloadGnbId);
            Ptr<Packet> initSuccPakcet = Create<Packet>(500);
            initSuccPakcet->AddHeader(cfRadioHeader);

            SendPacketToUe(ueId, initSuccPakcet);

            NS_LOG_INFO("Gnb " << m_mmWaveEnbNetDevice->GetCellId()
                               << " send forwarded Init Success to UE " << ueId);
        }
        else if (cfX2Header.GetMessageType() == CfX2Header::TaskResult)
        {
            // NS_LOG_INFO("Gnb " << m_mmWaveEnbNetDevice->GetCellId()
            //                    << " Recv forwarded Task Result " << cfX2Header.GetTaskId()
            //                    << " of UE " << cfX2Header.GetUeId() << " from gnb "
            //                    << cfX2Header.GetSourceGnbId());

            MultiPacketHeader mpHeader;
            bool receiveCompleted = false;

            // packet->RemoveHeader(mpHeader);
            packet->PeekHeader(mpHeader);

            receiveCompleted = m_resultManager->AddAndCheckPacket(cfX2Header.GetUeId(),
                                                                  cfX2Header.GetTaskId(),
                                                                  mpHeader.GetPacketId(),
                                                                  mpHeader.GetTotalPacketNum());
            if (receiveCompleted)
            {
                // NS_LOG_DEBUG("Recv result of "
                //              << "UE " << cfX2Header.GetUeId() << " TASK "
                //              << cfX2Header.GetTaskId());
                m_recvForwardedResultTrace(cfX2Header.GetUeId(),
                                           cfX2Header.GetTaskId(),
                                           Simulator::Now().GetTimeStep(),
                                           RecvForwardedResult,
                                           None);
            }

            NS_ASSERT(cfX2Header.GetTargetGnbId() == m_mmWaveEnbNetDevice->GetCellId());

            CfRadioHeader cfRadioHeader;
            cfRadioHeader.SetMessageType(CfRadioHeader::TaskResult);
            cfRadioHeader.SetTaskId(cfX2Header.GetTaskId());
            cfRadioHeader.SetUeId(cfX2Header.GetUeId());
            cfRadioHeader.SetGnbId(cfX2Header.GetSourceGnbId());

            packet->AddHeader(cfRadioHeader);
            // Ptr<Packet> resultPacket = Create<Packet>(500);
            // resultPacket->AddHeader(cfRadioHeader);

            SendPacketToUe(cfX2Header.GetUeId(), packet);
            // m_downlinkTrace(cfX2Header.GetUeId(),
            //                 cfX2Header.GetTaskId(),
            //                 Simulator::Now().GetTimeStep());
        }
        else if (cfX2Header.GetMessageType() == CfX2Header::MigrationData)
        {
            auto ueId = cfX2Header.GetUeId();
            auto sourceGnbId = cfX2Header.GetSourceGnbId();

            MultiPacketHeader mpHeader;
            bool receiveCompleted = false;

            packet->RemoveHeader(mpHeader);

            receiveCompleted = m_appDataManager->AddAndCheckPacket(sourceGnbId,
                                                                   ueId,
                                                                   mpHeader.GetPacketId(),
                                                                   mpHeader.GetTotalPacketNum());
            if (receiveCompleted)
            {
                NS_LOG_INFO("Gnb " << m_mmWaveEnbNetDevice->GetCellId()
                                   << " recv migration data of UE " << ueId << " from gNB "
                                   << sourceGnbId);
                UpdateUeState(ueId, UeState::Initializing);

                Simulator::Schedule(MilliSeconds(m_initDelay),
                                    &GnbCfApplication::CompleteMigrationAtTarget,
                                    this,
                                    ueId,
                                    sourceGnbId);
            }
        }
        else if (cfX2Header.GetMessageType() == CfX2Header::MigrationDone)
        {
            NS_LOG_INFO("Migration of UE " << cfX2Header.GetUeId() << " done, delete old info");
            auto ueId = cfX2Header.GetUeId();
            UpdateUeState(ueId, UeState::Over);
            m_cfUnit->DeleteUe(ueId);
        }
        else if (cfX2Header.GetMessageType() == CfX2Header::TerminateCommand)
        {
            uint64_t ueId = cfX2Header.GetUeId();
            UpdateUeState(ueId, UeState::Over);
            m_cfUnit->DeleteUe(ueId);
            NS_LOG_INFO("UE " << ueId << " terminate the service");
        }
        else if (cfX2Header.GetMessageType() == CfX2Header::RefuseInform)
        {
            uint64_t ueId = cfX2Header.GetUeId();

            CfRadioHeader cfRadioHeader;
            cfRadioHeader.SetMessageType(CfRadioHeader::RefuseInform);
            cfRadioHeader.SetUeId(ueId);

            packet->AddHeader(cfRadioHeader);

            SendPacketToUe(ueId, packet);
        }
        else
        {
            NS_FATAL_ERROR("Gnb " << m_mmWaveEnbNetDevice->GetCellId()
                                  << " receive a packet without valid type.");
        }
    }
}

void
GnbCfApplication::MigrateUeService(uint64_t ueId, uint64_t targetGnbId)
{
    NS_LOG_FUNCTION(this);

    UpdateUeState(ueId, UeState::Migrating);
    uint32_t packetNum = ceil(double(m_appSize) / m_defaultPacketSize);

    for (uint32_t n = 1; n <= packetNum; n++)
    {
        MultiPacketHeader mpHeader;
        mpHeader.SetPacketId(n);
        mpHeader.SetTotalpacketNum(packetNum);

        CfX2Header cfX2Header;
        cfX2Header.SetMessageType(CfX2Header::MigrationData);
        cfX2Header.SetUeId(ueId);
        cfX2Header.SetSourceGnbId(m_mmWaveEnbNetDevice->GetCellId());
        cfX2Header.SetTargetGnbId(targetGnbId);

        Ptr<Packet> p = Create<Packet>(m_defaultPacketSize);
        p->AddHeader(mpHeader);
        p->AddHeader(cfX2Header);

        SendPacketToOtherGnb(targetGnbId, p);
    }
}

void
GnbCfApplication::CompleteMigrationAtTarget(uint64_t ueId, uint64_t oldGnbId)
{
    m_cfUnit->AddNewUe(ueId);
    UpdateUeState(ueId, UeState::Serving);

    // TODO not harmonious
    Ptr<Packet> migraDonePacket = Create<Packet>(500);
    CfX2Header migraCfX2Header;
    migraCfX2Header.SetMessageType(CfX2Header::MigrationDone);
    migraCfX2Header.SetUeId(ueId);
    migraCfX2Header.SetSourceGnbId(m_mmWaveEnbNetDevice->GetCellId());
    migraCfX2Header.SetTargetGnbId(oldGnbId);
    migraDonePacket->AddHeader(migraCfX2Header);
    SendPacketToOtherGnb(oldGnbId, migraDonePacket);

    SendInitSuccessToConnectedGnb(ueId);
}

void
GnbCfApplication::SendNewUeReport(uint64_t ueId)
{
    NS_LOG_FUNCTION(this << ueId);
}

void
GnbCfApplication::AssignUe(uint64_t ueId, uint64_t offloadPointId)
{
    uint64_t ueAccessGnbId =
        m_cfranSystemInfo->GetUeInfo(ueId).m_mcUeNetDevice->GetMmWaveTargetEnb()->GetCellId();
    // NS_ASSERT(ueAccessGnbId == m_mmWaveEnbNetDevice->GetCellId());

    if (ueAccessGnbId != m_mmWaveEnbNetDevice->GetCellId())
    {
        NS_FATAL_ERROR("UE " << ueId << " access " << ueAccessGnbId << " Control msg sent to "
                             << m_mmWaveEnbNetDevice->GetCellId());
    }

    if (offloadPointId == m_mmWaveEnbNetDevice->GetCellId())
    {
        if (m_ueState.find(ueId) != m_ueState.end())
        {
            NS_LOG_UNCOND("Repeated initialization with redundant instructions");
        }
        else
        {
            // NS_LOG_DEBUG("Process UEs' init requests locally");
            UpdateUeState(ueId, UeState::Initializing);
            m_cfUnit->AddNewUe(ueId);
            SendInitSuccessToUserFromGnb(ueId);
            UpdateUeState(ueId, UeState::Serving);
        }
    }
    else
    {
        if (m_cfranSystemInfo->GetOffladPointType(offloadPointId) == CfranSystemInfo::Gnb)
        {
            // NS_LOG_INFO("Forward init request to gNB " << offloadPointId);
            CfX2Header x2Hd;
            x2Hd.SetMessageType(CfX2Header::InitRequest);
            x2Hd.SetSourceGnbId(ueAccessGnbId);
            x2Hd.SetTargetGnbId(offloadPointId);
            x2Hd.SetUeId(ueId);
            Ptr<Packet> p = Create<Packet>(500);
            p->AddHeader(x2Hd);

            SendPacketToOtherGnb(offloadPointId, p);
        }
        else if (m_cfranSystemInfo->GetOffladPointType(offloadPointId) == CfranSystemInfo::Remote)
        {
            // NS_LOG_INFO("Instruct users to offload tasks to remote server " << offloadPointId);
            CfRadioHeader cfrHd;
            cfrHd.SetMessageType(CfRadioHeader::OffloadCommand);
            cfrHd.SetGnbId(offloadPointId);
            cfrHd.SetUeId(ueId);
            Ptr<Packet> p = Create<Packet>(500);
            p->AddHeader(cfrHd);

            SendPacketToUe(ueId, p);
        }
    }
}

void
GnbCfApplication::BuildAndSendE2Report()
{
    NS_LOG_FUNCTION(this);

    // NS_LOG_DEBUG(this << "m_e2term->GetSubscriptionState(): " <<
    // m_e2term->GetSubscriptionState());
    if (m_e2term != nullptr && !m_e2term->GetSubscriptionState())
    {
        Simulator::Schedule(MilliSeconds(100), &GnbCfApplication::BuildAndSendE2Report, this);
        return;
    }

    // E2Termination::RicSubscriptionRequest_rval_s params = m_e2term->GetSubscriptionPara();

    std::string plmId = "111";
    std::string gnbId = std::to_string(m_mmWaveEnbNetDevice->GetCellId());
    uint16_t cellId = m_mmWaveEnbNetDevice->GetCellId();

    cJSON* msg = cJSON_CreateObject();
    cJSON_AddStringToObject(msg, "msgSource", "GnbCfApp");
    cJSON_AddNumberToObject(msg, "cfAppId", m_mmWaveEnbNetDevice->GetCellId());
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

        // Simulator::ScheduleWithContext(GetNode()->GetId(),
        //                                MilliSeconds(500),
        //                                &GnbCfApplication::BuildAndSendE2Report,
        //                                this);
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
        Ptr<KpmIndicationHeader> header =
            m_mmWaveEnbNetDevice->BuildRicIndicationHeader(plmId, gnbId, cellId);
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

    // NS_LOG_DEBUG("GnbCfApplication " << m_mmWaveEnbNetDevice->GetCellId()
    //                                  << " send indication message");
    Simulator::ScheduleWithContext(GetNode()->GetId(),
                                   Seconds(m_e2ReportPeriod),
                                   &GnbCfApplication::BuildAndSendE2Report,
                                   this);
    // Simulator::Schedule(MilliSeconds(500), &GnbCfApplication::BuildAndSendE2Report, this);
}

void
GnbCfApplication::ControlMessageReceivedCallback(E2AP_PDU_t* pdu)
{
    // NS_LOG_DEBUG ("Control Message Received, cellId is " << m_gnbNetDev->GetCellId ());
    // std::cout << "Control Message Received, cellId is " << m_mmWaveEnbNetDevice->GetCellId()
    //           << std::endl;
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

            cJSON* json = cJSON_Parse((const char*)ie->value.choice.RICcontrolMessage.buf);
            if (json == nullptr)
            {
                NS_LOG_ERROR("Parsing json failed");
            }
            else
            {
                // NS_LOG_DEBUG("Get available json");
                cJSON* uePolicy = nullptr;
                cJSON_ArrayForEach(uePolicy, json)
                {
                    cJSON* imsi = cJSON_GetObjectItemCaseSensitive(uePolicy, "imsi");
                    cJSON* offloadPointId =
                        cJSON_GetObjectItemCaseSensitive(uePolicy, "offloadPointId");
                    if (cJSON_IsNumber(imsi) && cJSON_IsNumber(offloadPointId))
                    {
                        NS_LOG_DEBUG("Recv ctrl policy for imsi "
                                     << " to cfNode " << offloadPointId->valueint);
                        // Policy uePolicy;
                        // uePolicy.m_ueId = imsi->valueint;
                        // uePolicy.m_offloadPointId = offloadPointId->valueint;
                        // m_policy.push(uePolicy);
                    }
                }
            }
            break;
        }
        default:
            break;
        }
    }

    // for (auto it = m_ueState.begin(); it != m_ueState.end(); it++)
    // {
    //     uint64_t ueId = it->first;
    //     AssignUe(ueId, 6);
    // }
}

void
GnbCfApplication::ExecuteCommands()
{
    while (!m_policy.empty())
    {
        Policy uePolicy = m_policy.front();
        m_policy.pop();

        uint64_t ueId = uePolicy.m_ueId;
        Action action = uePolicy.m_action;

        if (action == Action::StartService)
        {
            NS_LOG_INFO("GnbCfApp " << m_mmWaveEnbNetDevice->GetCellId() << " start service for UE "
                                    << ueId);
            uint64_t ueConnectingGnbId = m_cfranSystemInfo->GetUeInfo(ueId)
                                             .m_mcUeNetDevice->GetMmWaveTargetEnb()
                                             ->GetCellId();
            uint64_t hereGnbId = m_mmWaveEnbNetDevice->GetCellId();
            UpdateUeState(ueId, UeState::Initializing);
            m_cfUnit->AddNewUe(ueId);
            UpdateUeState(ueId, UeState::Serving);

            if (ueConnectingGnbId == hereGnbId)
            {
                SendInitSuccessToUserFromGnb(ueId);
            }
            else
            {
                SendInitSuccessToConnectedGnb(ueId);
            }
        }

        else if (action == Action::StopService)
        {
            NS_LOG_INFO("GnbCfApp " << m_mmWaveEnbNetDevice->GetCellId() << " stop service for UE "
                                    << ueId);
            UpdateUeState(ueId, UeState::Over);
            m_cfUnit->DeleteUe(ueId);
        }

        else if (action == Action::RefuseService)
        {
            NS_LOG_INFO("GnbCfApp " << m_mmWaveEnbNetDevice->GetCellId()
                                    << " send refuse information to UE " << ueId);
            if (m_ueState.find(ueId) != m_ueState.end())
            {
                UpdateUeState(ueId, UeState::Over);
                m_cfUnit->DeleteUe(ueId);
            }

            SendRefuseInformationToUe(ueId);
        }
        // AssignUe(uePolicy.m_ueId, uePolicy.m_offloadPointId);
    }

    Simulator::Schedule(MilliSeconds(5), &GnbCfApplication::ExecuteCommands, this);
}

void
GnbCfApplication::SendUeEventMessage(uint64_t ueId, CfranSystemInfo::UeRandomAction action)
{
    NS_LOG_FUNCTION(this);

    cJSON* msg = cJSON_CreateObject();
    cJSON_AddStringToObject(msg, "msgSource", "GnbCfApp");
    cJSON_AddNumberToObject(msg, "cfAppId", m_mmWaveEnbNetDevice->GetCellId());

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
        std::string gnbId = std::to_string(m_mmWaveEnbNetDevice->GetCellId());
        uint16_t cellId = m_mmWaveEnbNetDevice->GetCellId();

        Ptr<KpmIndicationHeader> header =
            m_mmWaveEnbNetDevice->BuildRicIndicationHeader(plmId, gnbId, cellId);
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

    NS_LOG_INFO("GnbCfApplication " << m_mmWaveEnbNetDevice->GetCellId()
                                    << " send event message of UE " << ueId);
}

void
GnbCfApplication::SendRefuseInformationToUe(uint64_t ueId)
{
    NS_LOG_FUNCTION(this);
    uint64_t ueConnectingGnbId =
        m_cfranSystemInfo->GetUeInfo(ueId).m_mcUeNetDevice->GetMmWaveTargetEnb()->GetCellId();
    uint64_t hereGnbId = m_mmWaveEnbNetDevice->GetCellId();

    Ptr<Packet> refusePacket = Create<Packet>(500);

    if (ueConnectingGnbId == hereGnbId)
    {
        CfRadioHeader cfrHeader;
        cfrHeader.SetMessageType(CfRadioHeader::RefuseInform);
        cfrHeader.SetGnbId(hereGnbId);
        refusePacket->AddHeader(cfrHeader);

        SendPacketToUe(ueId, refusePacket);
    }
    else
    {
        CfX2Header cfX2Header;
        cfX2Header.SetMessageType(CfX2Header::RefuseInform);
        cfX2Header.SetSourceGnbId(hereGnbId);
        cfX2Header.SetTargetGnbId(ueConnectingGnbId);
        cfX2Header.SetUeId(ueId);

        refusePacket->AddHeader(cfX2Header);

        SendPacketToOtherGnb(ueConnectingGnbId, refusePacket);
    }
}

void
GnbCfApplication::RecvFromCustomSocket()
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
            NS_LOG_DEBUG(" GnbCfApp " << m_mmWaveEnbNetDevice->GetCellId()
                                      << " Recv Policy message: " << buffer);
            PrasePolicyMessage(json);
        }
    }
}

void
GnbCfApplication::PrasePolicyMessage(cJSON* json)
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

} // namespace ns3
