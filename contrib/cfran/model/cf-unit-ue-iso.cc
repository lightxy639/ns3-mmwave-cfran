#include "cf-unit-ue-iso.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("CfUnitUeIso");

NS_OBJECT_ENSURE_REGISTERED(CfUnitUeIso);

TypeId
CfUnitUeIso::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::CfUnitUeIso").SetParent<CfUnit>().AddConstructor<CfUnitUeIso>();

    return tid;
}

CfUnitUeIso::CfUnitUeIso()
{
    NS_LOG_FUNCTION(this);
}

CfUnitUeIso::~CfUnitUeIso()
{
    NS_LOG_FUNCTION(this);
}

void
CfUnitUeIso::DoDispose()
{
    NS_LOG_FUNCTION(this);
}

void
CfUnitUeIso::LoadUeTask(uint64_t ueId, UeTaskModel ueTask)
{
    NS_LOG_FUNCTION(this << ueId << ueTask.m_taskId);
    auto it = m_ueTask.find(ueId);
    if (it != m_ueTask.end())
    {
        if (it->second.size() == 0 && !m_busy[ueId])
        {
            ExecuteUeTask(ueId, ueTask);
            NS_LOG_DEBUG("CfUnit " << m_id << " execute (UE, task): " << ueId << " "
                                   << ueTask.m_taskId << " directly");
        }
        else
        {
            NS_LOG_DEBUG("CfUnit " << m_id << " Push the (UE, task): " << ueId << " "
                                   << ueTask.m_taskId << " into queue");
            it->second.push(ueTask);
        }
    }
    else
    {
        NS_FATAL_ERROR("No UE info in this CfUnit");
    }
}

void
CfUnitUeIso::AddNewUe(uint64_t ueId)
{
    NS_LOG_FUNCTION(this << ueId);
    auto itTask = m_ueTask.find(ueId);
    auto itAllo = m_cfAllocation.find(ueId);

    NS_ASSERT(itTask == m_ueTask.end() && itAllo == m_cfAllocation.end());

    std::queue<UeTaskModel> ueTaskQueue;
    m_ueTask[ueId] = ueTaskQueue;
    m_cfAllocation[ueId] = CfModel(m_cf.m_cfType, 0);
    m_busy[ueId] = false;

    ReAllocateCf();
}

void
CfUnitUeIso::DeleteUe(uint64_t ueId)
{
    NS_LOG_FUNCTION(this << ueId);
    auto itTask = m_ueTask.find(ueId);
    auto itAllo = m_cfAllocation.find(ueId);
    auto itBusy = m_busy.find(ueId);
    NS_ASSERT(itTask != m_ueTask.end() && itAllo != m_cfAllocation.end() && itBusy != m_busy.end());

    m_ueTask.erase(itTask);
    m_cfAllocation.erase(itAllo);
    m_busy.erase(itBusy);

    ReAllocateCf();
}

void
CfUnitUeIso::ExecuteUeTask(uint64_t ueId, UeTaskModel ueTask)
{
    NS_LOG_FUNCTION(this);

    auto it = m_cfAllocation.find(ueId);
    if (it != m_cfAllocation.end())
    {
        CfModel ueCf = it->second;
        double executeLatency = 1000 * ueTask.m_cfLoad / ueCf.m_cfCapacity; // ms
        Simulator::Schedule(MilliSeconds(executeLatency),
                            &CfUnitUeIso::EndUeTask,
                            this,
                            ueId,
                            ueTask);
        m_busy[ueId] = true;
        NS_LOG_DEBUG("The computing latency of (UE, Task) " << ueId << " " << ueTask.m_taskId
                                                            << " is " << executeLatency << "ms");

        m_computingTaskTrace(ueId, ueTask.m_taskId, Simulator::Now().GetTimeStep());
    }
    else
    {
        NS_FATAL_ERROR("No UE info in this CfUnit");
    }
}

void
CfUnitUeIso::EndUeTask(uint64_t ueId, UeTaskModel ueTask)
{
    NS_LOG_FUNCTION(this);

    m_cfApplication->RecvTaskResult(ueId, ueTask);
    m_busy[ueId] = false;
    auto it = m_ueTask.find(ueId);
    if (it != m_ueTask.end())
    {
        if (it->second.size() > 0)
        {
            NS_LOG_DEBUG("Execute next task in queue.");
            ExecuteUeTask(ueId, it->second.front());
            it->second.pop();
        }
        else
        {
            NS_LOG_DEBUG("No other task in queue.");
        }
    }
}

void
CfUnitUeIso::ReAllocateCf()
{
    uint16_t ueNum = m_cfAllocation.size();

    for (auto it = m_cfAllocation.begin(); it != m_cfAllocation.end(); it++)
    {
        it->second = m_cf / ueNum;
        NS_LOG_DEBUG("UE " << it->first << " get " << m_cf / ueNum << "TFLOPS");
    }
}

// void
// CfUnitUeIso::InformTaskExecuting(uint64_t ueId, UeTaskModel ueTask)
// {
//     m_cfApplication->recvExecutingInform(ueId, ueTask);
// }

} // namespace ns3