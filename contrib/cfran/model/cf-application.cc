#include "cf-application.h"

#include <ns3/multi-packet-manager.h>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("CfApplication");
NS_OBJECT_ENSURE_REGISTERED(CfApplication);

TypeId
CfApplication::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::CfApplication")
            .SetParent<Application>()
            .AddAttribute("Port",
                          "Port on which we listen for incoming packets.",
                          UintegerValue(100),
                          MakeUintegerAccessor(&CfApplication::m_port),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("CfranSystemInfomation",
                          "Global user information in cfran scenario",
                          PointerValue(),
                          MakePointerAccessor(&CfApplication::m_cfranSystemInfo),
                          MakePointerChecker<CfranSystemInfo>())
            .AddAttribute("CfUnit",
                          "Corresponding CfUnit",
                          PointerValue(),
                          MakePointerAccessor(&CfApplication::m_cfUnit),
                          MakePointerChecker<CfUnit>())
            .AddAttribute("CfE2eCalculator",
                          "CfE2eCalculator instance",
                          PointerValue(),
                          MakePointerAccessor(&CfApplication::m_cfE2eCalaulator),
                          MakePointerChecker<CfE2eCalculator>())
            .AddTraceSource("RecvRequest",
                            "Recv UE task request of any UE directly",
                            MakeTraceSourceAccessor(&CfApplication::m_recvRequestTrace),
                            "ns3::UlRequestRx::TracedCallback")
            .AddTraceSource("AddTask",
                            "Add UE task to cfUnit queue",
                            MakeTraceSourceAccessor(&CfApplication::m_addTaskTrace),
                            "ns3::TaskQueuePush::TracedCallback")
            .AddTraceSource("GetResult",
                            "Get task result directly",
                            MakeTraceSourceAccessor(&CfApplication::m_getResultTrace),
                            "ns3::TaskCompleted::TracedCallback")
            .AddTraceSource("SendResult",
                            "Send UE task result to UE",
                            MakeTraceSourceAccessor(&CfApplication::m_sendResultTrace),
                            "ns3::ResultTx::TracedCallback");

    return tid;
}

CfApplication::CfApplication()
    : m_socket(nullptr),
      m_defaultPacketSize(1200),
      m_initDelay(200)
{
    m_multiPacketManager = CreateObject<MultiPacketManager>();
}

CfApplication::~CfApplication()
{
    NS_LOG_FUNCTION(this);
}

void
CfApplication::DoDispose()
{
    NS_LOG_FUNCTION(this);

    m_socket->Close();
}

void
CfApplication::SetCfUnit(Ptr<CfUnit> cfUnit)
{
    m_cfUnit = cfUnit;
    // this->SetAttribute("CfUnit", PointerValue(cfUnit));
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