#include "ue-cf-application.h"

#include "multi-packet-header.h"
#include "task-request-header.h"

#include <ns3/intercept-tag.h>
#include <ns3/log.h>
#include <ns3/lte-pdcp-tag.h>
#include <ns3/simulator.h>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("UeCfApplication");

NS_OBJECT_ENSURE_REGISTERED(UeCfApplication);

TypeId
UeCfApplication::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::UeCfApplication")
            .SetParent<Application>()
            .AddAttribute("CfranSystemInfomation",
                          "Global user information in cfran scenario",
                          PointerValue(),
                          MakePointerAccessor(&UeCfApplication::m_cfranSystemInfo),
                          MakePointerChecker<CfranSystemInfo>())
            // .AddAttribute("OffloadPort",
            //               "Port on which we listen for incoming packets.",
            //               UintegerValue(100),
            //               MakeUintegerAccessor(&UeCfApplication::m_offloadPort),
            //               MakeUintegerChecker<uint16_t>())
            .AddAttribute("RandomArrivalDeparture",
                        "If true, users will arrive and leave randomly",
                        BooleanValue(false),
                        MakeBooleanAccessor(&UeCfApplication::m_randomArrivalDeparture),
                        MakeBooleanChecker())
            .AddAttribute("UeGnbPort",
                          "The port used by the UE's socket to gnb in the cfran scenario",
                          UintegerValue(1234),
                          MakeUintegerAccessor(&UeCfApplication::m_ueGnbPort),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("UeRemotePort",
                          "The port used by the UE's socket to remote server in the cfran scenario",
                          UintegerValue(2345),
                          MakeUintegerAccessor(&UeCfApplication::m_ueRemotePort),
                          MakeUintegerChecker<uint16_t>())
            // .AddAttribute("CfE2eBuffer",
            //               "CfE2eBuffer instance",
            //               PointerValue(),
            //               MakePointerAccessor(&UeCfApplication::m_cfE2eBuffer),
            //               MakePointerChecker<CfE2eBuffer>())
            .AddAttribute("CfTimeBuffer",
                          "CfTimeBuffer instance",
                          PointerValue(),
                          MakePointerAccessor(&UeCfApplication::m_cfTimeBuffer),
                          MakePointerChecker<CfTimeBuffer>())
            .AddAttribute("CfE2eCalculator",
                          "CfE2eCalculator instance",
                          PointerValue(),
                          MakePointerAccessor(&UeCfApplication::m_cfE2eCalculator),
                          MakePointerChecker<CfE2eCalculator>())
            .AddTraceSource("SendRequest",
                            "Send task request through wireless channel",
                            MakeTraceSourceAccessor(&UeCfApplication::m_sendRequestTrace),
                            "ns3::UlTaskTransmission::TracedCallback")
            .AddTraceSource("RecvResult",
                            "Recv task result through wireless channel",
                            MakeTraceSourceAccessor(&UeCfApplication::m_recvResultTrace),
                            "ns3::DlResultTransmission::TracedCallback");

    return tid;
}

UeCfApplication::UeCfApplication()
    : m_ueId(0),
      m_taskId(0),
      m_active(false),
      m_socket(0),
      m_minSize(1000),
      //   m_requestDataSize(500000),
      m_uploadPacketSize(1000),
      m_period(40),
      m_accessGnbId(0),
      m_offloadPointId(0)
{
    NS_LOG_FUNCTION(this);
    m_downlinkResultManager = CreateObject<MultiPacketManager>();
}

UeCfApplication::~UeCfApplication()
{
    NS_LOG_FUNCTION(this);
}

void
UeCfApplication::InitGnbSocket()
{
    if (m_accessGnbId == m_mcUeNetDev->GetMmWaveTargetEnb()->GetCellId() && m_gnbSocket != nullptr)
    {
        return;
    }
    else if (m_gnbSocket != nullptr)
    {
        m_gnbSocket->Close();
    }

    m_accessGnbId = m_mcUeNetDev->GetMmWaveTargetEnb()->GetCellId();

    Ipv4Address gnbIp = m_cfranSystemInfo->GetCellInfo(m_accessGnbId).m_ipAddrToUe;
    uint16_t gnbPort = m_cfranSystemInfo->GetCellInfo(m_accessGnbId).m_portToUe;

    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    m_gnbSocket = Socket::CreateSocket(GetNode(), tid);
    InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_ueGnbPort);
    if (m_gnbSocket->Bind(local) == -1)
    {
        NS_FATAL_ERROR("Failed to bind socket");
    }

    m_gnbSocket->Connect(InetSocketAddress(gnbIp, gnbPort));

    NS_LOG_DEBUG("UE Init gnbSocket to " << gnbIp << ":" << gnbPort);
}

void
UeCfApplication::InitRemoteSocket()
{
    NS_ASSERT(m_cfranSystemInfo->GetOffladPointType(m_offloadPointId) ==
              CfranSystemInfo::OffloadPointType::Remote);

    if (m_remoteSocket != nullptr)
    {
        m_remoteSocket->Close();
    }

    Ipv4Address remoteIp = m_cfranSystemInfo->GetRemoteInfo(m_offloadPointId).m_ipAddr;
    uint16_t remotePort = m_cfranSystemInfo->GetRemoteInfo(m_offloadPointId).m_port;

    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    m_remoteSocket = Socket::CreateSocket(GetNode(), tid);
    InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_ueRemotePort);

    if (m_remoteSocket->Bind(local) == -1)
    {
        NS_FATAL_ERROR("Failed to bind socket");
    }

    m_remoteSocket->Connect(InetSocketAddress(remoteIp, remotePort));

    m_remoteSocket->SetRecvCallback(MakeCallback(&UeCfApplication::HandleRead, this));

    NS_LOG_DEBUG("Ue Init remoteSocket to " << remoteIp << ":" << remotePort);
}

void
UeCfApplication::StartApplication()
{
    NS_LOG_FUNCTION(this);

    if(!m_randomArrivalDeparture)
    {
        m_active = true;
        Simulator::Schedule(MilliSeconds(100), &UeCfApplication::SendInitRequest, this);
    }
    else
    {
        ChangeStatus();
    }
}

void
UeCfApplication::StopApplication()
{
    NS_LOG_FUNCTION(this);
}

void
UeCfApplication::DoDispose()
{
    NS_LOG_FUNCTION(this);

    // if (!m_socket)
    // {
    //     m_socket->Close();
    // }
}

void
UeCfApplication::SendInitRequest()
{
    NS_LOG_FUNCTION(this);

    Ptr<Socket> reqSocket;
    bool rlcIntercept;

    if (m_offloadPointId == 0 ||
        m_cfranSystemInfo->GetOffladPointType(m_offloadPointId) == CfranSystemInfo::Gnb)
    {
        InitGnbSocket();
        reqSocket = m_gnbSocket;
        rlcIntercept = true;
        NS_LOG_DEBUG("UE " << m_ueId << " send init request to cell "
                           << m_mcUeNetDev->GetMmWaveTargetEnb()->GetCellId());
    }

    else if (m_cfranSystemInfo->GetOffladPointType(m_offloadPointId) == CfranSystemInfo::Remote)
    {
        InitRemoteSocket();
        reqSocket = m_remoteSocket;
        rlcIntercept = false;
        NS_LOG_DEBUG("UE " << m_ueId << " send init request to remote server " << m_offloadPointId);
    }

    CfRadioHeader cfRadioHeader;
    cfRadioHeader.SetMessageType(CfRadioHeader::InitRequest);
    cfRadioHeader.SetUeId(m_ueId);
    cfRadioHeader.SetTaskId(m_taskId);

    Ptr<Packet> p = Create<Packet>(m_minSize);
    p->AddHeader(cfRadioHeader);

    if (rlcIntercept)
    {
        InterceptTag interceptTag;
        p->AddByteTag(interceptTag);
    }

    // std::ofstream out_file("PacketTag.txt");
    // p->PrintPacketTags(out_file);
    if (reqSocket->Send(p) < 0)
    {
        NS_LOG_ERROR("InitRequest error.");
    }
}

void
UeCfApplication::SendTaskRequest()
{
    NS_LOG_FUNCTION(this);

    Ptr<Socket> reqSocket = nullptr;
    // In the NSA scenario within the platform, data packets need to be intercepted in the RLC layer
    // of gNB in order to be processed by applications on gNB
    bool rlcIntercept = false;
    if (m_cfranSystemInfo->GetOffladPointType(m_offloadPointId) == CfranSystemInfo::Gnb)
    {
        NS_LOG_DEBUG("Using gnbSocekt");
        InitGnbSocket(); // Used for situations where switching occurs
        reqSocket = m_gnbSocket;
        rlcIntercept = true;
    }
    else
    {
        // InitRemoteSocket();
        reqSocket = m_remoteSocket;
        rlcIntercept = false;
    }

    uint64_t requestDataSize = m_cfranSystemInfo->GetUeInfo(m_ueId).m_taskModel.m_uplinkSize;
    uint32_t packetNum = std::ceil((float)requestDataSize / m_uploadPacketSize);
    for (uint32_t n = 1; n <= packetNum; n++)
    {
        // double packetInterval = 10; // us

        MultiPacketHeader mpHeader;
        mpHeader.SetPacketId(n);
        mpHeader.SetTotalpacketNum(packetNum);

        CfRadioHeader cfRadioHeader;
        cfRadioHeader.SetMessageType(CfRadioHeader::TaskRequest);
        cfRadioHeader.SetUeId(m_ueId);
        cfRadioHeader.SetTaskId(m_taskId);
        cfRadioHeader.SetGnbId(m_offloadPointId);

        Ptr<Packet> p = Create<Packet>(m_minSize);
        p->AddHeader(mpHeader);
        p->AddHeader(cfRadioHeader);

        // Simulator::Schedule(MicroSeconds((n-1) * packetInterval),
        // &UeCfApplication::SendPacketToGnb, this, p);
        if (rlcIntercept)
        {
            InterceptTag interceptTag;
            p->AddByteTag(interceptTag);
        }
        // if (reqSocket->Send(p) < 0)
        if (reqSocket->Send(p) < 0)
        {
            NS_FATAL_ERROR("Error in sending task request.");
        }
    }

    OffloadPosition offType = None;
    if (m_cfranSystemInfo->GetOffladPointType(m_offloadPointId) == CfranSystemInfo::Gnb)
    {
        offType = m_offloadPointId == m_accessGnbId ? LocalGnb : OtherGnb;
    }
    else
    {
        offType = RemoteServer;
    }
    m_sendRequestTrace(m_ueId,
                       m_taskId,
                       Simulator::Now().GetTimeStep(),
                       TimeType::SendRequest,
                       offType);

    NS_LOG_INFO("UE " << m_ueId << " send task request " << m_taskId << " to cell "
                      << m_offloadPointId);

    m_taskId++;
    Simulator::Schedule(MilliSeconds(m_period), &UeCfApplication::SendTaskRequest, this);
}

void
UeCfApplication::RecvTaskResult(Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this);
}

void
UeCfApplication::HandleRead(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    Ptr<Packet> packet;
    Address from;
    Address localAddress;
    while ((packet = socket->RecvFrom(from)))
    {
        socket->GetSockName(localAddress);
        //   m_rxTrace (packet);
        //   m_rxTraceWithAddresses (packet, from, localAddress);
        if (packet->GetSize() > 0)
        {
            this->RecvFromNetwork(packet);
        }
    }
}

void
UeCfApplication::RecvFromNetwork(Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this);
    // NS_LOG_DEBUG("p->GetSize(): "<< p->GetSize());
    CfRadioHeader cfRadioHeader;
    p->RemoveHeader(cfRadioHeader);
    TaskRequestHeader taskReq;
    if (cfRadioHeader.GetMessageType() == CfRadioHeader::InitSuccess)
    {
        uint64_t offloadPointId = cfRadioHeader.GetGnbId();
        if (m_cfranSystemInfo->GetOffladPointType(offloadPointId) == CfranSystemInfo::Gnb)
        {
            NS_LOG_INFO("Init Success in gNB " << offloadPointId);
        }
        else if (m_cfranSystemInfo->GetOffladPointType(offloadPointId) == CfranSystemInfo::Remote)
        {
            NS_LOG_INFO("Init Success in Remote server " << offloadPointId);
        }
        m_offloadPointId = offloadPointId;
        Simulator::Schedule(Seconds(0), &UeCfApplication::SendTaskRequest, this);
    }

    else if (cfRadioHeader.GetMessageType() == CfRadioHeader::OffloadCommand)
    {
        uint64_t offloadPointId = cfRadioHeader.GetGnbId();
        NS_ASSERT(m_cfranSystemInfo->GetOffladPointType(offloadPointId) == CfranSystemInfo::Remote);

        NS_LOG_DEBUG("UE recv decision offloading to remote server " << offloadPointId);

        if (m_offloadPointId != offloadPointId)
        {
            NS_LOG_DEBUG("Old offload id is " << m_offloadPointId);
            m_offloadPointId = offloadPointId;
            Simulator::Schedule(Seconds(0), &UeCfApplication::SendInitRequest, this);
        }
    }

    else if (cfRadioHeader.GetMessageType() == CfRadioHeader::TaskResult)
    {
        auto sourceId = cfRadioHeader.GetGnbId();
        auto taskId = cfRadioHeader.GetTaskId();

        MultiPacketHeader mpHeader;
        p->RemoveHeader(mpHeader);

        // PdcpTag pdcpTag;
        // p->FindFirstMatchingByteTag(pdcpTag);

        if (m_cfranSystemInfo->GetOffladPointType(m_offloadPointId) == CfranSystemInfo::Remote)
        {
            if (m_downlinkResultManager->IsNewFile(sourceId, taskId))
            {
                PdcpTag pdcpTag;
                if (p->FindFirstMatchingByteTag(pdcpTag))
                {
                    m_cfTimeBuffer->UpdateTimeBuffer(m_ueId,
                                                     taskId,
                                                     pdcpTag.GetSenderTimestamp().GetTimeStep(),
                                                     RecvForwardedResult,
                                                     None);
                    NS_LOG_DEBUG("Recv first packet " << " UE " << m_ueId << " task " << taskId << " packetId " << mpHeader.GetPacketId());
                }
                else
                {
                    NS_FATAL_ERROR("No valid pdcp tag");
                }
            }
        }
        // NS_LOG_DEBUG("Recv packet " << " UE " << m_ueId << " task " << taskId << " packetId " << mpHeader.GetPacketId());
        // NS_LOG_DEBUG("PDCP Latency " << Simulator::Now().GetTimeStep() - pdcpTag.GetSenderTimestamp().GetTimeStep());
        bool recvCompleted =
            m_downlinkResultManager->AddAndCheckPacket(sourceId,
                                                       taskId,
                                                       mpHeader.GetPacketId(),
                                                       mpHeader.GetTotalPacketNum());
        if (recvCompleted)
        {
            if (m_cfranSystemInfo->GetOffladPointType(m_offloadPointId) == CfranSystemInfo::Gnb)
            {
                NS_LOG_INFO("UE " << m_ueId << " Recv task result " << cfRadioHeader.GetTaskId()
                                  << " from gnb " << cfRadioHeader.GetGnbId());
            }
            else if (m_cfranSystemInfo->GetOffladPointType(m_offloadPointId) ==
                     CfranSystemInfo::Remote)
            {
                NS_LOG_INFO("UE " << m_ueId << " Recv task result " << cfRadioHeader.GetTaskId()
                                  << " from remote server " << cfRadioHeader.GetGnbId());
            }
            m_recvResultTrace(cfRadioHeader.GetUeId(),
                              cfRadioHeader.GetTaskId(),
                              Simulator::Now().GetTimeStep(),
                              RecvResult,
                              None);
            // m_rxResultTrace(m_ueId, cfRadioHeader.GetTaskId(), Simulator::Now().GetTimeStep());
            E2eTrace(cfRadioHeader);
        }
    }
    else
    {
        NS_FATAL_ERROR("Unexecpted message type");
    }
}

void
UeCfApplication::SendPacketToGnb(Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this);

    if (m_socket->Send(p) < 0)
    {
        NS_FATAL_ERROR("Error in sending task request.");
    }
}

void
UeCfApplication::SetUeId(uint64_t id)
{
    m_ueId = id;
}

void
UeCfApplication::HandlePacket(Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this);

    NS_LOG_DEBUG("VR packet arrived.");
}

void
UeCfApplication::SetMcUeNetDevice(Ptr<mmwave::McUeNetDevice> mcUeNetDev)
{
    m_mcUeNetDev = mcUeNetDev;
}

void
UeCfApplication::E2eTrace(CfRadioHeader cfRHd)
{
    // uint64_t
    NS_LOG_FUNCTION(this);

    uint64_t ueId = cfRHd.GetUeId();
    uint64_t taskId = cfRHd.GetTaskId();
    uint64_t offloadId = cfRHd.GetGnbId();
    uint64_t connectingGnbId = m_mcUeNetDev->GetMmWaveTargetEnb()->GetCellId();

    TimeData timeData = m_cfTimeBuffer->GetTimeData(ueId, taskId);

    if (timeData.m_pos == LocalGnb)
    {
        NS_LOG_DEBUG("UE " << ueId << " LocalGnb");
        uint64_t upWlDelay = timeData.m_recvRequest - timeData.m_sendRequest;
        uint64_t queueDelay = timeData.m_processTask - timeData.m_addTask;
        uint64_t processDelay = timeData.m_sendResult - timeData.m_processTask;
        uint64_t dnWlDelay = timeData.m_recvResult - timeData.m_sendResult;

        m_cfE2eCalculator
            ->UpdateDelayStats(ueId, upWlDelay, 0, queueDelay, processDelay, 0, dnWlDelay);
    }

    else if (timeData.m_pos == OtherGnb)
    {
        NS_LOG_DEBUG("UE " << ueId << " OtherGnb");
        uint64_t upWlDelay = timeData.m_recvRequestToBeForwarded - timeData.m_sendRequest;
        uint64_t upWdDelay = timeData.m_recvRequest - timeData.m_recvRequestToBeForwarded;
        uint64_t queueDelay = timeData.m_processTask - timeData.m_addTask;
        uint64_t processDelay = timeData.m_sendResult - timeData.m_processTask;
        uint64_t dnWdDelay = timeData.m_recvForwardedResult - timeData.m_sendResult;
        uint64_t dnWlDelay = timeData.m_recvResult - timeData.m_recvForwardedResult;

        m_cfE2eCalculator->UpdateDelayStats(ueId,
                                            upWlDelay,
                                            upWdDelay,
                                            queueDelay,
                                            processDelay,
                                            dnWdDelay,
                                            dnWlDelay);
    }

    else if (timeData.m_pos == RemoteServer)
    {
        NS_LOG_DEBUG("UE " << ueId << " RemoteServer");
        uint64_t upWdDelay = m_cfranSystemInfo->GetWiredLatencyInfo().m_s1ULatency +
                             m_cfranSystemInfo->GetWiredLatencyInfo().m_x2Latency +
                             m_cfranSystemInfo->GetRemoteInfo(offloadId).m_hostGwLatency * 1e6;
        // NS_LOG_DEBUG("Remote server " << offloadId << " upWd")
        uint64_t upWlDelay = timeData.m_recvRequest - timeData.m_sendRequest - upWdDelay;
        uint64_t queueDelay = timeData.m_processTask - timeData.m_addTask;
        uint64_t processDelay = timeData.m_sendResult - timeData.m_processTask;

        // uint64_t dnWdDelay = m_cfranSystemInfo->GetWiredLatencyInfo().m_s1ULatency +
        //                      m_cfranSystemInfo->GetWiredLatencyInfo().m_x2Latency +
        //                      m_cfranSystemInfo->GetRemoteInfo(offloadId).m_hostGwLatency * 1e6;
        // uint64_t dnWlDelay = timeData.m_recvResult - timeData.m_sendResult - dnWdDelay;
        uint64_t dnWlDelay = timeData.m_recvResult - timeData.m_recvForwardedResult;

        uint64_t dnWdDelay = timeData.m_recvResult - timeData.m_sendResult - dnWlDelay;

        m_cfE2eCalculator->UpdateDelayStats(ueId,
                                            upWlDelay,
                                            upWdDelay,
                                            queueDelay,
                                            processDelay,
                                            dnWdDelay,
                                            dnWlDelay);
        NS_LOG_DEBUG("UE " << ueId << " TASK " << taskId << " dnWlDelay " << dnWlDelay);
    }

    m_cfTimeBuffer->RemoveTimeData(ueId, taskId);

    // uint64_t upWlDelay = m_cfE2eBuffer->GetUplinkWirelessDelay(ueId, taskId, true);
    // uint64_t upWdDelay = m_cfE2eBuffer->GetUplinkWiredDelay(ueId, taskId, true);
    // uint64_t queueDelay = m_cfE2eBuffer->GetQueueDelay(ueId, taskId, true);
    // uint64_t computingDelay = m_cfE2eBuffer->GetComputingDelay(ueId, taskId, true);
    // uint64_t dnWdDelay = m_cfE2eBuffer->GetDownlinkWiredDelay(ueId, taskId, true);
    // uint64_t dnWlDelay = m_cfE2eBuffer->GetDownlinkWirelessDelay(ueId, taskId, true);

    // m_cfE2eCalculator->UpdateDelayStats(ueId,
    //                                     upWlDelay,
    //                                     upWdDelay,
    //                                     queueDelay,
    //                                     computingDelay,
    //                                     dnWdDelay,
    //                                     dnWlDelay);
}

void
UeCfApplication::ChangeStatus()
{
    
}

} // namespace ns3