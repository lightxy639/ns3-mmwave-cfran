#include "vr-server.h"

#include <ns3/log.h>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("VrServer");

NS_OBJECT_ENSURE_REGISTERED(VrServer);

TypeId
VrServer::GetTypeId()
{
    static TypeId tid = TypeId("ns3::VrServer")
                            .SetParent<CfApplication>()
                            .AddConstructor<VrServer>()
                            .AddAttribute("CfranSystemInfo",
                                          "Global user information in cfran scenario",
                                          PointerValue(),
                                          MakePointerAccessor(&VrServer::m_cfranSystemInfo),
                                          MakePointerChecker<CfranSystemInfo>());
    return tid;
}

VrServer::VrServer()
{
    NS_LOG_FUNCTION(this);
}

VrServer::~VrServer()
{
    NS_LOG_FUNCTION(this);
}

void
VrServer::DoDispose()
{
    NS_LOG_FUNCTION(this);
    Application::DoDispose();
}

void
VrServer::StartApplication()
{
    NS_LOG_FUNCTION(this);
}

void
VrServer::StopApplication()
{
    NS_LOG_FUNCTION(this);

    auto it = m_ueApplicationInfo.begin();
    for (; it != m_ueApplicationInfo.end(); it++)
    {
        Simulator::Cancel(it->second.m_eventId);
    }
}

void
VrServer::StartServiceForImsi(uint64_t imsi)
{
    // Start to generate tasks for UE imsi
    Ptr<UniformRandomVariable> startRng = CreateObject<UniformRandomVariable>();
    uint32_t startOffset = startRng->GetInteger(1, 10);

    auto eventId =
        Simulator::Schedule(MilliSeconds(startOffset), &VrServer::Handle4Imsi, this, imsi);

    UeApplicationInfo info;
    info.m_eventId = eventId;
    info.m_frameHandle = 0;
    info.m_frameSent = 0;

    // Create app info for ue imsi
    AddApplicationInfoForImsi(imsi, info);
}

void
VrServer::Handle4Imsi(uint64_t imsi)
{
    // TODO
    auto it = m_ueApplicationInfo.find(imsi);
    NS_ASSERT(it != m_ueApplicationInfo.end());

    UeTaskModel task;
    task.m_taskId = it->second.m_frameHandle;
    // TODO Get the task attributes of UE imsi
    auto ueInfo = m_cfranSystemInfo->GetUeInfo(imsi);
    task.m_cfLoad = ueInfo.m_taskModel.m_cfLoad;
    task.m_cfRequired = m_cfUnit->GetCf() / m_ueApplicationInfo.size(); // TODO operator overload
    task.m_deadline = ueInfo.m_taskModel.m_deadline;
    task.m_application = this;

    NS_LOG_DEBUG("The application pointer is " << task.m_application);
    // m_cfUnit->AddNewUeTaskForSchedule(imsi, task);
    this->LoadTaskToCfUnit(imsi, task);

    it->second.m_frameHandle++;
    it->second.m_eventId = Simulator::Schedule(MilliSeconds(ueInfo.m_taskPeriodity),
                                               &VrServer::Handle4Imsi,
                                               this,
                                               imsi);
}

void
VrServer::StopServiceForImsi(uint64_t imsi)
{
    NS_LOG_FUNCTION(this << imsi);

    auto it = m_ueApplicationInfo.find(imsi);
    NS_ASSERT(it != m_ueApplicationInfo.end());

    Simulator::Cancel(it->second.m_eventId);
    DeleteApplicationInfoForImsi(imsi);
}

void
VrServer::AddApplicationInfoForImsi(uint64_t imsi, UeApplicationInfo info)
{
    NS_LOG_FUNCTION(this << imsi);

    auto it = m_ueApplicationInfo.find(imsi);

    NS_ASSERT(it == m_ueApplicationInfo.end());

    m_ueApplicationInfo.insert(std::pair<uint64_t, UeApplicationInfo>(imsi, info));
}

void
VrServer::DeleteApplicationInfoForImsi(uint64_t imsi)
{
    NS_LOG_FUNCTION(this << imsi);

    auto it = m_ueApplicationInfo.find(imsi);

    NS_ASSERT(it != m_ueApplicationInfo.end());

    m_ueApplicationInfo.erase(it);
}

void
VrServer::LoadTaskToCfUnit(uint64_t id, UeTaskModel ueTask)
{
    NS_LOG_FUNCTION(this);
    m_cfUnit->AddNewUeTaskForSchedule(id, ueTask);
}

void
VrServer::RecvTaskResult(uint64_t id, UeTaskModel ueTask)
{
    NS_LOG_FUNCTION(this << "UE " << id << " TASK " << ueTask.m_taskId);
}

} // namespace ns3
