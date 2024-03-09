#include "ue-cf-application.h"
#include "task-request-header.h"

#include <ns3/log.h>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("UeCfApplication");

NS_OBJECT_ENSURE_REGISTERED(UeCfApplication);

TypeId
UeCfApplication::GetTypeId()
{
    static TypeId tid = TypeId("ns3::UeCfApplication").SetParent<Application>();

    return tid;
}

UeCfApplication::UeCfApplication()
    : m_ueId(0),
      m_taskId(0),
      m_socket(0),
      m_minSize(23)
{
    NS_LOG_FUNCTION(this);
}

UeCfApplication::~UeCfApplication()
{
    NS_LOG_FUNCTION(this);
}

void
UeCfApplication::SetOffloadAddress(Address address, uint32_t port)
{
    m_offloadAddress = address;
    m_offloadPort = port;
}

void
UeCfApplication::SwitchOffloadAddress(Address newAddress, uint32_t newPort)
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

    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    m_socket = Socket::CreateSocket(GetNode(), tid);
    if (Ipv4Address::IsMatchingType(m_offloadAddress) == true)
    {
        if (m_socket->Bind() == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        }
        m_socket->Connect(
            InetSocketAddress(Ipv4Address::ConvertFrom(m_offloadAddress), m_offloadPort));
    }
    else if (Ipv6Address::IsMatchingType(m_offloadAddress) == true)
    {
        if (m_socket->Bind6() == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        }
        m_socket->Connect(
            Inet6SocketAddress(Ipv6Address::ConvertFrom(m_offloadAddress), m_offloadPort));
    }
    else if (InetSocketAddress::IsMatchingType(m_offloadAddress) == true)
    {
        if (m_socket->Bind() == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        }
        m_socket->Connect(m_offloadAddress);
    }
    else if (Inet6SocketAddress::IsMatchingType(m_offloadAddress) == true)
    {
        if (m_socket->Bind6() == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        }
        m_socket->Connect(m_offloadAddress);
    }
    else
    {
        NS_ASSERT_MSG(false, "Incompatible address type: " << m_offloadAddress);
    }
}

void
UeCfApplication::StartApplication()
{
    NS_LOG_FUNCTION(this);

    InitSocket();
}

void
UeCfApplication::SendTaskRequest()
{
    NS_LOG_FUNCTION(this);
    TaskRequestHeader taskReq;
    taskReq.SetUeId(m_ueId);
    taskReq.SetTaskId(m_taskId);

    Ptr<Packet> p = Create<Packet>(m_minSize);
    p->AddHeader(taskReq);

    if(m_socket->Send(p) >= 0)
    {
        m_taskId++;
    }
}

void
UeCfApplication::RecvTaskResult(Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this);
}
void
UeCfApplication::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  Address localAddress;
  while ((packet = socket->RecvFrom (from)))
    {
      socket->GetSockName (localAddress);
    //   m_rxTrace (packet);
    //   m_rxTraceWithAddresses (packet, from, localAddress);
      if (packet->GetSize () > 0)
        {
          RecvTaskResult (packet);
        }
    }
}
} // namespace ns3