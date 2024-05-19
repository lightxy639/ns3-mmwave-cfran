#include "remote-cf-application.h"

#include "cf-radio-header.h"
#include "multi-packet-header.h"

#include <ns3/log.h>

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
RemoteCfApplication::SendPacketToUe(uint64_t ueId, Ptr<Packet> packet)
{
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
}

void
RemoteCfApplication::StartApplication()
{
    NS_LOG_FUNCTION(this);

    if (!m_socket)
    {
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        m_socket = Socket::CreateSocket(GetNode(), tid);

        NS_LOG_DEBUG("m_port " << m_port);
        InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_port);
        if (m_socket->Bind(local) == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        }
    }

    m_socket->SetRecvCallback(MakeCallback(&RemoteCfApplication::RecvFromUe, this));
}

void
RemoteCfApplication::StopApplication()
{
    NS_LOG_FUNCTION(this);
}

} // namespace ns3