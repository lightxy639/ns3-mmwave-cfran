#include "gnb-cf-application.h"

#include "encode_e2apv1.hpp"

#include <ns3/cJSON.h>
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
            .AddTraceSource("ForwardRequest",
                            "Forward UE task request of UE",
                            MakeTraceSourceAccessor(&GnbCfApplication::m_forwardRequestTrace),
                            "ns3::UlRequestForwardTx::TracedCallback")
            .AddTraceSource("RecvForwardedRequest",
                            "Recv forwarded UE task request of UE",
                            MakeTraceSourceAccessor(&GnbCfApplication::m_recvForwardedRequestTrace),
                            "ns3::UlRequestForwardedRx::TracedCallback")
            .AddTraceSource("ForwardResult",
                            "Forward task result to corresponding gNB",
                            MakeTraceSourceAccessor(&GnbCfApplication::m_forwardResultTrace),
                            "ns3::ResultWiredDownlink::TracedCallback")
            .AddTraceSource("GetForwardedResult",
                            "Recv forwarded task result",
                            MakeTraceSourceAccessor(&GnbCfApplication::m_getForwardedResultTrace),
                            "ns3::ResultWiredDownlinkRx::TracedCallback");

    return tid;
}

GnbCfApplication::GnbCfApplication()
    : m_cfX2Port(4275),
      m_appSize(50000000)
{
    NS_LOG_FUNCTION(this);
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

Ptr<E2Termination>
GnbCfApplication::GetE2Termination() const
{
    return m_e2term;
}

void
GnbCfApplication::SetE2Termination(Ptr<E2Termination> e2term)
{
    m_e2term = e2term;
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
    NS_LOG_DEBUG("cell " << m_mmWaveEnbNetDevice->GetCellId() << " start application");

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
    Simulator::Schedule(Seconds(1.5), &GnbCfApplication::BuildAndSendE2Report, this);
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

    m_getResultTrace(ueId, ueTask.m_taskId, Simulator::Now().GetTimeStep());

    auto ueConnectedGnbId =
        m_cfranSystemInfo->GetUeInfo(ueId).m_mcUeNetDevice->GetMmWaveTargetEnb()->GetCellId();
    auto appGnbId = m_mmWaveEnbNetDevice->GetCellId();

    if (ueConnectedGnbId == appGnbId)
    {
        // SendTaskResultToUserFromGnb(ueId);
        m_sendResultTrace(ueId, ueTask.m_taskId, Simulator::Now().GetTimeStep());

        Ptr<Packet> resultPacket = Create<Packet>(500);

        CfRadioHeader echoHeader;
        echoHeader.SetUeId(ueId);
        echoHeader.SetMessageType(CfRadioHeader::TaskResult);
        echoHeader.SetGnbId(m_mmWaveEnbNetDevice->GetCellId());
        echoHeader.SetTaskId(ueTask.m_taskId);
        resultPacket->AddHeader(echoHeader);

        SendPacketToUe(ueId, resultPacket);

        // m_downlinkTrace(ueId, ueTask.m_taskId, Simulator::Now().GetTimeStep());
    }
    else
    {
        m_forwardResultTrace(ueId, ueTask.m_taskId, Simulator::Now().GetTimeStep());
        // SendTaskResultToConnectedGnb(ueId);
        Ptr<Packet> resultPacket = Create<Packet>(500);

        CfX2Header cfX2Header;
        cfX2Header.SetMessageType(CfX2Header::TaskResult);
        cfX2Header.SetSourceGnbId(m_mmWaveEnbNetDevice->GetCellId());
        cfX2Header.SetTargetGnbId(ueConnectedGnbId);
        cfX2Header.SetUeId(ueId);
        cfX2Header.SetTaskId(ueTask.m_taskId);

        resultPacket->AddHeader(cfX2Header);

        SendPacketToOtherGnb(ueConnectedGnbId, resultPacket);
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
            UpdateUeState(cfRadioHeader.GetUeId(), UeState::Initializing);
            m_cfUnit->AddNewUe(cfRadioHeader.GetUeId());
            SendInitSuccessToUserFromGnb(cfRadioHeader.GetUeId());
            UpdateUeState(cfRadioHeader.GetUeId(), UeState::Serving);
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
                    m_multiPacketManager->AddAndCheckPacket(ueId,
                                                            taskId,
                                                            mpHeader.GetPacketId(),
                                                            mpHeader.GetTotalPacketNum());
                if (receiveCompleted)
                {
                    NS_LOG_INFO("Gnb " << m_mmWaveEnbNetDevice->GetCellId() << " Recv task request "
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
                // NS_LOG_INFO("Gnb " << hereGnbId << " Send the request of UE "
                //                    << cfRadioHeader.GetUeId() << " to other gNB.");

                MultiPacketHeader mpHd;
                packet->PeekHeader(mpHd);
                bool receiveCompleted =
                    m_multiPacketManager->AddAndCheckPacket(ueId,
                                                            taskId,
                                                            mpHd.GetPacketId(),
                                                            mpHd.GetTotalPacketNum());
                if (receiveCompleted)
                {
                    m_recvRequestTrace(ueId, taskId, Simulator::Now().GetTimeStep());
                    m_forwardRequestTrace(ueId, taskId, Simulator::Now().GetTimeStep());
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

            UpdateUeState(cfX2Header.GetUeId(), UeState::Initializing);
            m_cfUnit->AddNewUe(cfX2Header.GetUeId());
            SendInitSuccessToConnectedGnb(cfX2Header.GetUeId());
            UpdateUeState(cfX2Header.GetUeId(), UeState::Serving);
        }
        else if (cfX2Header.GetMessageType() == CfX2Header::TaskRequest)
        {
            auto ueId = cfX2Header.GetUeId();
            auto taskId = cfX2Header.GetTaskId();

            MultiPacketHeader mpHeader;
            bool receiveCompleted = false;

            packet->RemoveHeader(mpHeader);
            receiveCompleted =
                m_multiPacketManager->AddAndCheckPacket(ueId + 100,
                                                        taskId,
                                                        mpHeader.GetPacketId(),
                                                        mpHeader.GetTotalPacketNum());
            if (receiveCompleted)
            {
                NS_LOG_INFO("Gnb " << m_mmWaveEnbNetDevice->GetCellId()
                                   << " Recv task request of UE " << cfX2Header.GetUeId()
                                   << " from gnb " << cfX2Header.GetSourceGnbId());
                m_recvForwardedRequestTrace(ueId, taskId, Simulator::Now().GetTimeStep());
                m_addTaskTrace(ueId, taskId, Simulator::Now().GetTimeStep());

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
        }
        else if (cfX2Header.GetMessageType() == CfX2Header::TaskResult)
        {
            NS_LOG_INFO("Gnb " << m_mmWaveEnbNetDevice->GetCellId()
                               << " Recv forwarded Task Result " << cfX2Header.GetTaskId()
                               << " of UE " << cfX2Header.GetUeId() << " from gnb "
                               << cfX2Header.GetSourceGnbId());
            m_getForwardedResultTrace(cfX2Header.GetUeId(),
                                      cfX2Header.GetTaskId(),
                                      Simulator::Now().GetTimeStep());
            m_sendResultTrace(cfX2Header.GetUeId(),
                              cfX2Header.GetTaskId(),
                              Simulator::Now().GetTimeStep());

            NS_ASSERT(cfX2Header.GetTargetGnbId() == m_mmWaveEnbNetDevice->GetCellId());

            CfRadioHeader cfRadioHeader;
            cfRadioHeader.SetMessageType(CfRadioHeader::TaskResult);
            cfRadioHeader.SetTaskId(cfX2Header.GetTaskId());
            cfRadioHeader.SetUeId(cfX2Header.GetUeId());
            cfRadioHeader.SetGnbId(cfX2Header.GetSourceGnbId());
            Ptr<Packet> resultPacket = Create<Packet>(500);
            resultPacket->AddHeader(cfRadioHeader);

            SendPacketToUe(cfX2Header.GetUeId(), resultPacket);
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

            receiveCompleted =
                m_multiPacketManager->AddAndCheckPacket(sourceGnbId + 200,
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
GnbCfApplication::BuildAndSendE2Report()
{
    NS_LOG_FUNCTION(this);

    NS_LOG_DEBUG(this << "m_e2term->GetSubscriptionState(): " << m_e2term->GetSubscriptionState());
    if (!m_e2term->GetSubscriptionState())
    {
        Simulator::Schedule(MilliSeconds(500), &GnbCfApplication::BuildAndSendE2Report, this);
        return;
    }

    E2Termination::RicSubscriptionRequest_rval_s params = m_e2term->GetSubscriptionPara();

    std::string plmId = "111";
    std::string gnbId = std::to_string(m_mmWaveEnbNetDevice->GetCellId());
    uint16_t cellId = m_mmWaveEnbNetDevice->GetCellId();

    Ptr<KpmIndicationHeader> header =
        m_mmWaveEnbNetDevice->BuildRicIndicationHeader(plmId, gnbId, cellId);

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

        cJSON* ueStatsMsg = cJSON_CreateObject();
        cJSON_AddNumberToObject(ueStatsMsg, "Id", ueId);

        cJSON_AddNumberToObject(ueStatsMsg, "upWlMea", upWlDelay[0] / 1e6);
        cJSON_AddNumberToObject(ueStatsMsg, "upWlStd", upWlDelay[1] / 1e6);
        cJSON_AddNumberToObject(ueStatsMsg, "upWlMin", upWlDelay[2] / 1e6);
        cJSON_AddNumberToObject(ueStatsMsg, "upWlMax", upWlDelay[3] / 1e6);

        cJSON_AddNumberToObject(ueStatsMsg, "upWdMea", upWdDelay[0] / 1e6);
        cJSON_AddNumberToObject(ueStatsMsg, "upWdStd", upWdDelay[1] / 1e6);
        cJSON_AddNumberToObject(ueStatsMsg, "upWdMin", upWdDelay[2] / 1e6);
        cJSON_AddNumberToObject(ueStatsMsg, "upWdMax", upWdDelay[3] / 1e6);

        cJSON_AddNumberToObject(ueStatsMsg, "queMea", queueDelay[0] / 1e6);
        cJSON_AddNumberToObject(ueStatsMsg, "queStd", queueDelay[1] / 1e6);
        cJSON_AddNumberToObject(ueStatsMsg, "queMin", queueDelay[2] / 1e6);
        cJSON_AddNumberToObject(ueStatsMsg, "queMax", queueDelay[3] / 1e6);

        cJSON_AddNumberToObject(ueStatsMsg, "compMea", compDelay[0] / 1e6);
        cJSON_AddNumberToObject(ueStatsMsg, "compStd", compDelay[1] / 1e6);
        cJSON_AddNumberToObject(ueStatsMsg, "compMin", compDelay[2] / 1e6);
        cJSON_AddNumberToObject(ueStatsMsg, "compMax", compDelay[3] / 1e6);

        cJSON_AddNumberToObject(ueStatsMsg, "dnWdMea", dnWdDelay[0] / 1e6);
        cJSON_AddNumberToObject(ueStatsMsg, "dnWdStd", dnWdDelay[1] / 1e6);
        cJSON_AddNumberToObject(ueStatsMsg, "dnWdMin", dnWdDelay[2] / 1e6);
        cJSON_AddNumberToObject(ueStatsMsg, "dnWdMax", dnWdDelay[3] / 1e6);

        cJSON_AddNumberToObject(ueStatsMsg, "dnWlMea", dnWlDelay[0] / 1e6);
        cJSON_AddNumberToObject(ueStatsMsg, "dnWlStd", dnWlDelay[1] / 1e6);
        cJSON_AddNumberToObject(ueStatsMsg, "dnWlMin", dnWlDelay[2] / 1e6);
        cJSON_AddNumberToObject(ueStatsMsg, "dnWlMax", dnWlDelay[3] / 1e6);

        std::string testString = cJSON_PrintUnformatted(ueStatsMsg);
        // const char* testString = cJSON_PrintUnformatted(ueStatsMsg).c_str();
        NS_LOG_DEBUG(testString.c_str());

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
            (uint8_t*)(testString.c_str()),
            //  duMsg->m_size
            strlen(testString.c_str())); // size of the encoded message
        m_e2term->SendE2Message(pdu_du_ue);
        delete pdu_du_ue;

        // Simulator::ScheduleWithContext(GetNode()->GetId(),
        //                                MilliSeconds(500),
        //                                &GnbCfApplication::BuildAndSendE2Report,
        //                                this);
    }

    Simulator::Schedule(MilliSeconds(500), &GnbCfApplication::BuildAndSendE2Report, this);
}

} // namespace ns3
