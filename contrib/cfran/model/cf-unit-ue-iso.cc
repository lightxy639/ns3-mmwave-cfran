#include "cf-unit-ue-iso.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("CfUnitUeIso");

NS_OBJECT_ENSURE_REGISTERED(CfUnitUeIso);

TypeId
CfUnitUeIso::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CfUnitUeIso").SetParent<CfUnit>().AddConstructor<CfUnitUeIso>();

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
    NS_LOG_FUNCTION(this);
    auto it = m_ueTask.find(ueId);
    if (it != m_ueTask.end())
    {
        if (it->second.size() == 0)
        {
            ExecuteUeTask(ueId, ueTask);
            NS_LOG_DEBUG("No other in queue, execute it directly.");
        }
        else
        {
            it->second.push(ueTask);
            NS_LOG_DEBUG("Push the task into queue.");
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

    ReAllocateCf();
}

void
CfUnitUeIso::DeleteUe(uint64_t ueId)
{
    NS_LOG_FUNCTION(this << ueId);
    auto itTask = m_ueTask.find(ueId);
    auto itAllo = m_cfAllocation.find(ueId);
    NS_ASSERT(itTask != m_ueTask.end() && itAllo != m_cfAllocation.end());

    m_ueTask.erase(itTask);
    m_cfAllocation.erase(itAllo);

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
        Simulator::Schedule(MilliSeconds(executeLatency), &CfUnitUeIso::EndUeTask, this, ueId, ueTask);
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

    auto it = m_ueTask.find(ueId);
    if(it != m_ueTask.end())
    {
        if (it->second.size() > 0)
        {
            ExecuteUeTask(ueId, it->second.front());
            it->second.pop();
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
    }
}

}