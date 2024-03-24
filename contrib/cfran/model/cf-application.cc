#include "cf-application.h"

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
      m_cfX2Port(4275),
      m_appSize(50000000),
      m_defaultPacketSize(1200),
      m_initDelay(200)
{
    NS_LOG_FUNCTION(this);
    m_multiPacketManager = Create<MultiPacketManager>();
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
CfApplication::SendPakcetToUe(uint64_t ueId, Ptr<Packet> packet)
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
CfApplication::SendPacketToOtherGnb(uint64_t gnbId, Ptr<Packet> packet)
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
CfApplication::SendInitSuccessToUserFromGnb(uint64_t id)
{
    NS_LOG_FUNCTION(this);

    Ptr<Packet> resultPacket = Create<Packet>(500);

    CfRadioHeader echoHeader;
    echoHeader.SetMessageType(CfRadioHeader::InitSuccess);
    echoHeader.SetGnbId(m_mmWaveEnbNetDevice->GetCellId());

    resultPacket->AddHeader(echoHeader);

    SendPakcetToUe(id, resultPacket);
}

void
CfApplication::SendInitSuccessToConnectedGnb(uint64_t ueId)
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
    NS_LOG_FUNCTION(this << ueId << ueTask.m_taskId);

    auto ueConnectedGnbId =
        m_cfranSystemInfo->GetUeInfo(ueId).m_mcUeNetDevice->GetMmWaveTargetEnb()->GetCellId();
    auto appGnbId = m_mmWaveEnbNetDevice->GetCellId();

    if (ueConnectedGnbId == appGnbId)
    {
        // SendTaskResultToUserFromGnb(ueId);

        Ptr<Packet> resultPacket = Create<Packet>(500);

        CfRadioHeader echoHeader;
        echoHeader.SetMessageType(CfRadioHeader::TaskResult);
        echoHeader.SetGnbId(m_mmWaveEnbNetDevice->GetCellId());
        echoHeader.SetTaskId(ueTask.m_taskId);
        resultPacket->AddHeader(echoHeader);

        SendPakcetToUe(ueId, resultPacket);
    }
    else
    {
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
CfApplication::HandleRead(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this);
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
    NS_LOG_FUNCTION(this);
    Ptr<Packet> packet;
    Address from;
    Address localAddress;
    while (packet = socket->RecvFrom(from))
    {
        socket->GetSockName(localAddress);
        m_rxTrace(packet);
        m_rxTraceWithAddresses(packet, from, localAddress);

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
                    UeTaskModel ueTask = m_cfranSystemInfo->GetUeInfo(ueId).m_taskModel;
                    ueTask.m_taskId = cfRadioHeader.GetTaskId();
                    m_cfUnit->LoadUeTask(ueId, ueTask);
                }
            }
            else
            {
                // NS_LOG_INFO("Gnb " << hereGnbId << " Send the request of UE "
                //                    << cfRadioHeader.GetUeId() << " to other gNB.");
                CfX2Header x2Hd;
                x2Hd.SetMessageType(CfX2Header::TaskRequest);
                x2Hd.SetSourceGnbId(hereGnbId);
                x2Hd.SetTargetGnbId(offloadGnbId);
                x2Hd.SetUeId(ueId);
                x2Hd.SetTaskId(cfRadioHeader.GetTaskId());
                // Ptr<Packet> resultPacket = Create<Packet>(500);

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
        localCfSocket->Connect(InetSocketAddress(remoteIpAddr, m_cfX2Port));
        NS_ASSERT_MSG(m_cfX2InterfaceSockets.find(remoteCellId) == m_cfX2InterfaceSockets.end(),
                      "Mapping for remoteCellId = " << remoteCellId << " is already known");

        localCfSocket->SetRecvCallback(MakeCallback(&CfApplication::RecvFromOtherGnb, this));
        m_cfX2InterfaceSockets[remoteCellId] = Create<CfX2IfaceInfo>(remoteIpAddr, localCfSocket);
    }
}

void
CfApplication::RecvFromOtherGnb(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this);
    Ptr<Packet> packet;
    Address from;
    Address localAddress;

    while (packet = socket->RecvFrom(from))
    {
        socket->GetSockName(localAddress);
        m_rxTrace(packet);
        m_rxTraceWithAddresses(packet, from, localAddress);

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

            SendPakcetToUe(ueId, initSuccPakcet);
        }
        else if (cfX2Header.GetMessageType() == CfX2Header::TaskResult)
        {
            NS_LOG_INFO("Gnb " << m_mmWaveEnbNetDevice->GetCellId()
                               << " Recv forwarded Task Result " << cfX2Header.GetTaskId()
                               << " of UE " << cfX2Header.GetUeId() << " from gnb "
                               << cfX2Header.GetSourceGnbId());
            NS_ASSERT(cfX2Header.GetTargetGnbId() == m_mmWaveEnbNetDevice->GetCellId());

            CfRadioHeader cfRadioHeader;
            cfRadioHeader.SetMessageType(CfRadioHeader::TaskResult);
            cfRadioHeader.SetTaskId(cfX2Header.GetTaskId());
            cfRadioHeader.SetUeId(cfX2Header.GetUeId());
            cfRadioHeader.SetGnbId(cfX2Header.GetSourceGnbId());
            Ptr<Packet> resultPacket = Create<Packet>(500);
            resultPacket->AddHeader(cfRadioHeader);

            SendPakcetToUe(cfX2Header.GetUeId(), resultPacket);
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
                                    &CfApplication::CompleteMigrationAtTarget,
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
CfApplication::MigrateUeService(uint64_t ueId, uint64_t targetGnbId)
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
CfApplication::CompleteMigrationAtTarget(uint64_t ueId, uint64_t oldGnbId)
{
    m_cfUnit->AddNewUe(ueId);
    UpdateUeState(ueId, UeState::Serving);

    //TODO not harmonious
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
CfApplication::UpdateUeState(uint64_t id, UeState state)
{
    NS_LOG_FUNCTION(this << id << state);

    NS_LOG_INFO("UE " << id << " Stat changed to " << state);
    if (state == UeState::Initializing)
    {
        NS_ASSERT(m_ueState.find(id) == m_ueState.end());
        m_ueState[id] = UeState::Initializing;
    }
    else if (state == UeState::Serving)
    {
        NS_ASSERT(m_ueState[id] == UeState::Initializing);
        m_ueState[id] = UeState::Serving;
    }
    else if (state == UeState::Migrating)
    {
        NS_ASSERT(m_ueState[id] == UeState::Serving);
        m_ueState[id] = UeState::Migrating;
    }
    else if (state == UeState::Over)
    {
        NS_ASSERT(m_ueState.find(id) != m_ueState.end());
        m_ueState.erase(id);
    }
    else
    {
        NS_FATAL_ERROR("Unvalid state.");
    }
}
} // namespace ns3
