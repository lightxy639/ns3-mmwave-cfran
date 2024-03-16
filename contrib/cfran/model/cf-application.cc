#include "cf-application.h"

#include <ns3/cf-radio-header.h>
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
NS_LOG_COMPONENT_DEFINE("CfApplication");

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

NS_OBJECT_ENSURE_REGISTERED(CfApplication);

TypeId
CfApplication::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::CfApplication")
            .SetParent<Application>()
            .AddConstructor<CfApplication>()
            .AddAttribute("Port",
                          "Port on which we listen for incoming packets.",
                          UintegerValue(100),
                          MakeUintegerAccessor(&CfApplication::m_port),
                          MakeUintegerChecker<uint16_t>())
            .AddTraceSource("Rx",
                            "A packet has been received",
                            MakeTraceSourceAccessor(&CfApplication::m_rxTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("RxWithAddresses",
                            "A packet has been received",
                            MakeTraceSourceAccessor(&CfApplication::m_rxTraceWithAddresses),
                            "ns3::Packet::TwoAddressTracedCallback")
            .AddAttribute("CfranSystemInfomation",
                          "Global user information in cfran scenario",
                          PointerValue(),
                          MakePointerAccessor(&CfApplication::m_cfranSystemInfo),
                          MakePointerChecker<CfranSystemInfo>());

    return tid;
}

CfApplication::CfApplication()
    : m_socket(nullptr),
      m_cfX2Port(4275)
{
    NS_LOG_FUNCTION(this);
}

CfApplication::~CfApplication()
{
    NS_LOG_FUNCTION(this);
}

void
CfApplication::SetCfUnit(Ptr<CfUnit> cfUnit)
{
    m_cfUnit = cfUnit;
}

void
CfApplication::SetMmWaveEnbNetDevice(Ptr<mmwave::MmWaveEnbNetDevice> mmwaveEnbNetDev)
{
    m_mmWaveEnbNetDevice = mmwaveEnbNetDev;
}

void
CfApplication::RecvTaskRequest()
{
    NS_LOG_FUNCTION(this);
}

void
CfApplication::SendTaskResultToUserFromGnb(uint64_t id)
{
    NS_LOG_FUNCTION(this);

    Ptr<Packet> resultPacket = Create<Packet>(500);

    CfRadioHeader echoHeader;
    echoHeader.SetMessageType(CfRadioHeader::TaskResult);
    echoHeader.SetGnbId(m_mmWaveEnbNetDevice->GetCellId());

    resultPacket->AddHeader(echoHeader);
    // socket->SendTo(resultPacket, 0, from);
    PdcpTag pdcpTag(Simulator::Now());
    resultPacket->AddByteTag(pdcpTag);

    // Adding pdcptag is necessary since mc-ue-pdcp will remove 2-bit pdcptag
    LtePdcpHeader pdcpHeader;
    resultPacket->AddHeader(pdcpHeader);

    CfranSystemInfo::UeInfo ueInfo = m_cfranSystemInfo->GetUeInfo(id);
    Ptr<ns3::mmwave::McUeNetDevice> mcUeNetDev = ueInfo.m_mcUeNetDevice;

    Ptr<ns3::mmwave::MmWaveEnbNetDevice> ueMmWaveEnbNetDev = mcUeNetDev->GetMmWaveTargetEnb();
    NS_ASSERT(ueMmWaveEnbNetDev != nullptr);

    uint16_t lteRnti = mcUeNetDev->GetLteRrc()->GetRnti();
    Ptr<LteEnbNetDevice> lteEnbNetDev = mcUeNetDev->GetLteTargetEnb();
    auto drbMap = lteEnbNetDev->GetRrc()->GetUeManager(lteRnti)->GetDrbMap();
    uint32_t gtpTeid = (drbMap.begin()->second->m_gtpTeid);


    EpcX2RlcUser* epcX2RlcUser =
        ueMmWaveEnbNetDev->GetNode()->GetObject<EpcX2>()->GetX2RlcUserMap().find(gtpTeid)->second;

    NS_ASSERT(epcX2RlcUser != nullptr);
    if (epcX2RlcUser != nullptr)
    {
        EpcX2SapUser::UeDataParams params;

        params.gtpTeid = gtpTeid;
        params.ueData = resultPacket;
        PdcpTag pdcpTag(Simulator::Now());

        params.ueData->AddByteTag(pdcpTag);
        epcX2RlcUser->SendMcPdcpSdu(params);
    }
}

void
CfApplication::SendInitSuccessToUserFromGnb(uint64_t id)
{
    NS_LOG_FUNCTION(this);

    Ptr<Packet> resultPacket = Create<Packet>(500);

    CfRadioHeader echoHeader;
    echoHeader.SetMessageType(CfRadioHeader::InitSuccess);
    echoHeader.SetGnbId(m_mmWaveEnbNetDevice->GetCellId());

    resultPacket->AddHeader(echoHeader);
    // socket->SendTo(resultPacket, 0, from);
    PdcpTag pdcpTag(Simulator::Now());
    resultPacket->AddByteTag(pdcpTag);

    // Adding pdcptag is necessary since mc-ue-pdcp will remove 2-bit pdcptag
    LtePdcpHeader pdcpHeader;
    resultPacket->AddHeader(pdcpHeader);

    CfranSystemInfo::UeInfo ueInfo = m_cfranSystemInfo->GetUeInfo(id);
    Ptr<ns3::mmwave::McUeNetDevice> mcUeNetDev = ueInfo.m_mcUeNetDevice;

    Ptr<ns3::mmwave::MmWaveEnbNetDevice> ueMmWaveEnbNetDev = mcUeNetDev->GetMmWaveTargetEnb();
    NS_ASSERT(ueMmWaveEnbNetDev != nullptr);

    uint16_t lteRnti = mcUeNetDev->GetLteRrc()->GetRnti();
    Ptr<LteEnbNetDevice> lteEnbNetDev = mcUeNetDev->GetLteTargetEnb();
    auto drbMap = lteEnbNetDev->GetRrc()->GetUeManager(lteRnti)->GetDrbMap();
    uint32_t gtpTeid = (drbMap.begin()->second->m_gtpTeid);

    EpcX2RlcUser* epcX2RlcUser =
        ueMmWaveEnbNetDev->GetNode()->GetObject<EpcX2>()->GetX2RlcUserMap().find(gtpTeid)->second;

    NS_ASSERT(epcX2RlcUser != nullptr);
    if (epcX2RlcUser != nullptr)
    {
        EpcX2SapUser::UeDataParams params;

        params.gtpTeid = gtpTeid;
        params.ueData = resultPacket;
        PdcpTag pdcpTag(Simulator::Now());

        params.ueData->AddByteTag(pdcpTag);
        epcX2RlcUser->SendMcPdcpSdu(params);
    }
}

void
CfApplication::SendTaskResultToUserFromRemote(uint64_t id, Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(this);
}

void
CfApplication::StartApplication()
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
    // m_socket->SetRecvCallback(MakeCallback(&CfApplication::HandleRead, this));
    m_socket->SetRecvCallback(MakeCallback(&CfApplication::RecvFromUe, this));

    InitX2Info();
}

void
CfApplication::StopApplication()
{
    NS_LOG_FUNCTION(this);
}

void
CfApplication::DoDispose()
{
    m_socket->Close();
}

void
CfApplication::LoadTaskToCfUnit(uint64_t id, UeTaskModel ueTask)
{
    NS_LOG_FUNCTION(this);
}

void
CfApplication::RecvTaskResult(uint64_t ueId, UeTaskModel ueTask)
{
    NS_LOG_FUNCTION(this);

    SendTaskResultToUserFromGnb(ueId);
    NS_LOG_DEBUG("Gnb " << m_mmWaveEnbNetDevice->GetCellId() << " recv task result of (UE,Task) "
                        << ueId << " " << ueTask.m_taskId);
}

void
CfApplication::HandleRead(Ptr<Socket> socket)
{
    NS_LOG_UNCOND("CfApplication Receive IPv4 packet");
    Ptr<Packet> packet;
    Address from;
    Address localAddress;

    while ((packet = socket->RecvFrom(from)))
    {
        socket->GetSockName(localAddress);
        m_rxTrace(packet);
        m_rxTraceWithAddresses(packet, from, localAddress);
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
CfApplication::RecvFromUe(Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    Address from;
    Address localAddress;
    while ((packet = socket->RecvFrom(from)))
    {
        socket->GetSockName(localAddress);
        m_rxTrace(packet);
        m_rxTraceWithAddresses(packet, from, localAddress);

        CfRadioHeader cfRadioHeader;
        packet->RemoveHeader(cfRadioHeader);

        if (cfRadioHeader.GetMessageType() == CfRadioHeader::InitRequest)
        {
            NS_LOG_DEBUG("Gnb " << m_mmWaveEnbNetDevice->GetCellId() << "Recv init request of UE "
                                << cfRadioHeader.GetUeId());

            m_cfUnit->AddNewUe(cfRadioHeader.GetUeId());
            SendInitSuccessToUserFromGnb(cfRadioHeader.GetUeId());
        }
        else if (cfRadioHeader.GetMessageType() == CfRadioHeader::TaskRequest)
        {
            NS_LOG_DEBUG("Gnb " << m_mmWaveEnbNetDevice->GetCellId() << "Recv task request of UE "
                                << cfRadioHeader.GetUeId());
            auto ueId = cfRadioHeader.GetUeId();
            UeTaskModel ueTask = m_cfranSystemInfo->GetUeInfo(ueId).m_taskModel;
            ueTask.m_taskId = cfRadioHeader.GetTaskId();
            m_cfUnit->LoadUeTask(ueId, ueTask);
            // SendTaskResultToUserFromGnb(cfRadioHeader.GetUeId());
        }
        else
        {
            NS_FATAL_ERROR("Something wrong");
        }

        // NS_LOG_LOGIC("Echoing packet");
    }
}

void
CfApplication::InitX2Info()
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
        NS_ASSERT_MSG(m_cfX2InterfaceSockets.find(remoteCellId) == m_cfX2InterfaceSockets.end(),
                      "Mapping for remoteCellId = " << remoteCellId << " is already known");
        m_cfX2InterfaceSockets[remoteCellId] = Create<CfX2IfaceInfo>(remoteIpAddr, localCfSocket);
    }
}

} // namespace ns3
