#include "ue-cf-application.h"

#include "task-request-header.h"
#include "cf-radio-header.h"

#include <ns3/log.h>
#include <ns3/simulator.h>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("UeCfApplication");

NS_OBJECT_ENSURE_REGISTERED(UeCfApplication);

TypeId
UeCfApplication::GetTypeId()
{
    static TypeId tid = TypeId("ns3::UeCfApplication")
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
                                          MakeUintegerChecker<uint16_t>());

    return tid;
}

UeCfApplication::UeCfApplication()
    : m_ueId(0),
      m_taskId(0),
      m_socket(0),
      m_minSize(1000),
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

    CfRadioHeader cfRadioHeader;
    cfRadioHeader.SetMessageType(CfRadioHeader::TaskRequest);
    cfRadioHeader.SetUeId(m_ueId);
    cfRadioHeader.SetTaskId(m_taskId);
    cfRadioHeader.SetGnbId(m_offloadGnbId);

    Ptr<Packet> p = Create<Packet>(m_minSize);
    p->AddHeader(cfRadioHeader);
    if (m_socket->Send(p) >= 0)
    {
        m_taskId++;
        NS_LOG_INFO("UE " << m_ueId << " send task request to cell " << m_offloadGnbId);

    }
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
        NS_LOG_INFO("UE " << m_ueId << " Recv task result from gnb " << cfRadioHeader.GetGnbId());
    }
    else
    {
        NS_FATAL_ERROR("Unexecpted message type");
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

} // namespace ns3