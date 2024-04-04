#include "ue-cf-application.h"

#include "multi-packet-header.h"
#include "task-request-header.h"

#include <ns3/log.h>
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
            .AddAttribute("OffloadPort",
                          "Port on which we listen for incoming packets.",
                          UintegerValue(100),
                          MakeUintegerAccessor(&UeCfApplication::m_offloadPort),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("CfE2eBuffer",
                          "CfE2eBuffer instance",
                          PointerValue(),
                          MakePointerAccessor(&UeCfApplication::m_cfE2eBuffer),
                          MakePointerChecker<CfE2eBuffer>())
            .AddAttribute("CfE2eCalculator",
                          "CfE2eCalculator instance",
                          PointerValue(),
                          MakePointerAccessor(&UeCfApplication::m_cfE2eCalculator),
                          MakePointerChecker<CfE2eCalculator>())
            .AddTraceSource("TxRequest",
                            "Send task request through wireless channel",
                            MakeTraceSourceAccessor(&UeCfApplication::m_txRequestTrace),
                            "ns3::UlTaskTransmission::TracedCallback")
            .AddTraceSource("RxResult",
                            "Recv task result through wireless channel",
                            MakeTraceSourceAccessor(&UeCfApplication::m_rxResultTrace),
                            "ns3::DlResultTransmission::TracedCallback");

    return tid;
}

UeCfApplication::UeCfApplication()
    : m_ueId(0),
      m_taskId(0),
      m_socket(0),
      m_minSize(1000),
      m_requestDataSize(500000),
      m_uploadPacketSize(1000),
      m_period(40)
{
    NS_LOG_FUNCTION(this);
}

UeCfApplication::~UeCfApplication()
{
    NS_LOG_FUNCTION(this);
}

void
UeCfApplication::SetOffloadAddress(Ipv4Address address, uint32_t port)
{
    m_offloadAddress = address;
    m_offloadPort = port;
}

void
UeCfApplication::SwitchOffloadAddress(Ipv4Address newAddress, uint32_t newPort)
{
    if (newAddress != m_offloadAddress)
    {
        m_offloadAddress = newAddress;
        m_socket->Close();
        m_socket = 0;

        InitSocket();
    }
    m_offloadPort = newPort;
}

void
UeCfApplication::InitSocket()
{
    NS_ASSERT(!m_socket);

    uint16_t gnbId = m_mcUeNetDev->GetMmWaveTargetEnb()->GetCellId();
    Ipv4Address gnbIp = m_cfranSystemInfo->GetCellInfo(gnbId).m_ipAddrToUe;
    m_offloadAddress = gnbIp;

    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    m_socket = Socket::CreateSocket(GetNode(), tid);
    if (m_socket->Bind() == -1)
    {
        NS_FATAL_ERROR("Failed to bind socket");
    }

    m_socket->Connect(InetSocketAddress(gnbIp, m_offloadPort));
}

void
UeCfApplication::StartApplication()
{
    NS_LOG_FUNCTION(this);

    InitSocket();

    Simulator::Schedule(MilliSeconds(100), &UeCfApplication::SendInitRequest, this);
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

    if (!m_socket)
    {
        m_socket->Close();
    }
}

void
UeCfApplication::SendInitRequest()
{
    NS_LOG_FUNCTION(this);

    uint16_t gnbId = m_mcUeNetDev->GetMmWaveTargetEnb()->GetCellId();
    Ipv4Address gnbIp = m_cfranSystemInfo->GetCellInfo(gnbId).m_ipAddrToUe;

    if (gnbIp != m_offloadAddress)
    {
        SwitchOffloadAddress(gnbIp, m_offloadPort);
    }

    CfRadioHeader cfRadioHeader;
    cfRadioHeader.SetMessageType(CfRadioHeader::InitRequest);
    cfRadioHeader.SetUeId(m_ueId);
    cfRadioHeader.SetTaskId(m_taskId);

    Ptr<Packet> p = Create<Packet>(m_minSize);
    p->AddHeader(cfRadioHeader);
    if (m_socket->Send(p) >= 0)
    {
        NS_LOG_DEBUG("UE " << m_ueId << " send init request to cell " << gnbId);
    }
}

void
UeCfApplication::SendTaskRequest()
{
    NS_LOG_FUNCTION(this);

    uint16_t connectingGnbId = m_mcUeNetDev->GetMmWaveTargetEnb()->GetCellId();
    Ipv4Address connectingGnbIp = m_cfranSystemInfo->GetCellInfo(connectingGnbId).m_ipAddrToUe;

    if (connectingGnbIp != m_offloadAddress)
    {
        SwitchOffloadAddress(connectingGnbIp, m_offloadPort);
    }

    uint32_t packetNum = std::ceil(float(m_requestDataSize) / m_uploadPacketSize);
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
        cfRadioHeader.SetGnbId(m_offloadGnbId);

        Ptr<Packet> p = Create<Packet>(m_minSize);
        p->AddHeader(mpHeader);
        p->AddHeader(cfRadioHeader);

        // Simulator::Schedule(MicroSeconds((n-1) * packetInterval),
        // &UeCfApplication::SendPacketToGnb, this, p);
        if (m_socket->Send(p) < 0)
        {
            NS_FATAL_ERROR("Error in sending task request.");
        }
    }
    m_txRequestTrace(m_ueId, m_taskId, Simulator::Now().GetTimeStep());

    NS_LOG_INFO("UE " << m_ueId << " send task request " << m_taskId << " to cell "
                      << m_offloadGnbId);

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
            RecvTaskResult(packet);
        }
    }
}

void
UeCfApplication::RecvFromGnb(Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this);
    // NS_LOG_DEBUG("p->GetSize(): "<< p->GetSize());
    CfRadioHeader cfRadioHeader;
    p->RemoveHeader(cfRadioHeader);
    TaskRequestHeader taskReq;
    if (cfRadioHeader.GetMessageType() == CfRadioHeader::InitSuccess)
    {
        NS_LOG_INFO("Init Success in gNB " << cfRadioHeader.GetGnbId());
        m_offloadGnbId = cfRadioHeader.GetGnbId();
        Simulator::Schedule(Seconds(0), &UeCfApplication::SendTaskRequest, this);
    }
    else if (cfRadioHeader.GetMessageType() == CfRadioHeader::TaskResult)
    {
        NS_LOG_INFO("UE " << m_ueId << " Recv task result " << cfRadioHeader.GetTaskId()
                          << " from gnb " << cfRadioHeader.GetGnbId());
        m_rxResultTrace(cfRadioHeader.GetUeId(),
                        cfRadioHeader.GetTaskId(),
                        Simulator::Now().GetTimeStep());
        // m_rxResultTrace(m_ueId, cfRadioHeader.GetTaskId(), Simulator::Now().GetTimeStep());
        E2eTrace(cfRadioHeader);
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
    uint64_t offloadGnbId = cfRHd.GetGnbId();
    uint64_t connectingGnbId = m_mcUeNetDev->GetMmWaveTargetEnb()->GetCellId();

    uint64_t upWlDelay = m_cfE2eBuffer->GetUplinkWirelessDelay(ueId, taskId, true);
    uint64_t upWdDelay = m_cfE2eBuffer->GetUplinkWiredDelay(ueId, taskId, true);
    uint64_t queueDelay = m_cfE2eBuffer->GetQueueDelay(ueId, taskId, true);
    uint64_t computingDelay = m_cfE2eBuffer->GetComputingDelay(ueId, taskId, true);
    uint64_t dnWdDelay = m_cfE2eBuffer->GetDownlinkWiredDelay(ueId, taskId, true);
    uint64_t dnWlDelay = m_cfE2eBuffer->GetDownlinkWirelessDelay(ueId, taskId, true);

    m_cfE2eCalculator->UpdateDelayStats(ueId,
                                        upWlDelay,
                                        upWdDelay,
                                        queueDelay,
                                        computingDelay,
                                        dnWdDelay,
                                        dnWlDelay);
}

} // namespace ns3