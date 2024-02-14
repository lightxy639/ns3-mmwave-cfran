#include "cf-unit.h"

#include <ns3/boolean.h>
#include <ns3/log.h>
#include <ns3/node.h>
#include <ns3/simulator.h>

#include <iomanip>
#include <iostream>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("CfUnit");

NS_OBJECT_ENSURE_REGISTERED(CfUnit);

TypeId
CfUnit::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::CfUnit")
            .SetParent<Object>()
            .AddConstructor<CfUnit>()
            .AddAttribute("EnableAutoSchedule",
                          "If true,  CfUnit allocate computing force for tasks autonomously."
                          "Otherwise, CfUnit will obey the instructions of the application.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&CfUnit::m_enableAutoSchedule),
                          MakeBooleanChecker())
            .AddAttribute("CfModel",
                          "The Computing force of the CfUnit",
                          CfModelValue(CfModel()),
                          MakeCfModelAccessor(&CfUnit::m_cf),
                          MakeCfModelChecker());

    return tid;
}

void
CfUnit::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    Object::DoInitialize();
}

void
CfUnit::DoDispose()
{
    NS_LOG_FUNCTION(this);
    Object::DoDispose();
}

CfUnit::CfUnit()
{
    NS_LOG_FUNCTION(this);
    CfModel cf("GPU", 82.6);
    m_cf = cf;
}

CfUnit::CfUnit(CfModel cf)
    : m_cf(cf),
      m_freeCf(cf)
{
    NS_LOG_FUNCTION(this);
}

CfUnit::~CfUnit()
{
    NS_LOG_FUNCTION(this);
}

void
CfUnit::SetNode(Ptr<Node> node)
{
    NS_LOG_FUNCTION(this << node);
    m_node = node;
}

void
CfUnit::ScheduleTasks()
{
    NS_LOG_FUNCTION(this);
}

void
CfUnit::AddNewUeTaskForSchedule(uint64_t id, UeTaskModel ueTask)
{
    NS_LOG_FUNCTION(this << id << ueTask.m_taskId << ueTask.m_cfLoad);
    auto it = m_ueTask.find(id);
    if (it != m_ueTask.end())
    {
        NS_LOG_DEBUG("Add new task " << ueTask.m_taskId << " for UE " << id);

        NS_ASSERT(it->second.find(ueTask.m_taskId) ==
                  it->second.end()); // Ensure the uniqueness of the task id
        it->second.insert(std::pair<uint64_t, UeTaskModel>(ueTask.m_taskId, ueTask));
    }
    else
    {
        NS_LOG_DEBUG("Create new task map for UE " << id << std::endl);
        std::map<uint64_t, UeTaskModel> ueTaskMap;

        NS_LOG_DEBUG("Add new task " << ueTask.m_taskId << " for the task vector of UE " << id);
        ueTaskMap.insert(std::pair<uint64_t, UeTaskModel>(ueTask.m_taskId, ueTask));

        m_ueTask.insert(std::pair<uint64_t, std::map<uint64_t, UeTaskModel>>(id, ueTaskMap));
    }

    if (!m_enableAutoSchedule)
    {
        ScheduleNewTaskWithCommand(id, ueTask);
    }
}

void
CfUnit::DeleteUeTask(uint64_t id, UeTaskModel ueTask)
{
    NS_LOG_FUNCTION(this << id << ueTask.m_taskId);

    auto itUe = m_ueTask.find(id);
    NS_ASSERT(itUe != m_ueTask.end());

    auto itTask = itUe->second.find(ueTask.m_taskId);
    NS_ASSERT(itTask != itUe->second.end());

    itUe->second.erase(itTask);
}

void
CfUnit::AllocateCf(uint64_t id, UeTaskModel ueTask, CfModel cfModel)
{
    NS_LOG_FUNCTION(this << id << ueTask.m_taskId << cfModel.m_cfCapacity);

    auto it = m_cfAllocation.find(id);
    if (it != m_cfAllocation.end())
    {
        auto taskId = ueTask.m_taskId;
        auto itTask = it->second.find(taskId);
        NS_ASSERT(itTask == it->second.end());

        m_cfAllocation[id][ueTask.m_taskId] = cfModel;
    }
    else
    {
        m_cfAllocation[id][ueTask.m_taskId] = cfModel;
    }

    m_freeCf = m_freeCf - cfModel;

    NS_LOG_DEBUG("The free Cf of CfUnit " << m_id << " is " << m_freeCf.m_cfCapacity << " TFLOPS"
                                          << std::endl);
}

void
CfUnit::ScheduleNewTaskWithCommand(uint64_t id, UeTaskModel ueTask)
{
    NS_LOG_FUNCTION(this << id << ueTask.m_taskId);
    NS_ASSERT(m_freeCf >= ueTask.m_cfRequired);

    AllocateCf(id, ueTask, ueTask.m_cfRequired);

    ExecuteTask(id, ueTask);

    OutputCfAllocationInfo();
}

void
CfUnit::ExecuteTask(uint64_t id, UeTaskModel ueTask)
{
    NS_LOG_FUNCTION(this << id << ueTask.m_taskId);

    CfModel allocateCf = m_cfAllocation[id][ueTask.m_taskId];
    float allocatedCapacity = allocateCf.m_cfCapacity;

    float computingLatency = ueTask.m_cfLoad / allocatedCapacity * 1000; // ms

    NS_LOG_DEBUG("UE " << id << " task " << ueTask.m_taskId << " computing latency "
                       << computingLatency << "ms");

    Simulator::Schedule(MilliSeconds(computingLatency), &CfUnit::EndTask, this, id, ueTask);
}

void
CfUnit::FreeCf(uint64_t id, UeTaskModel ueTask)
{
    NS_LOG_FUNCTION(this << id << ueTask.m_taskId);

    auto itUe = m_cfAllocation.find(id);
    NS_ASSERT(itUe != m_cfAllocation.end());

    auto itTask = itUe->second.find(ueTask.m_taskId);

    NS_ASSERT(itTask != itUe->second.end());

    // Delete the info in allocation map and calculator free computing force
    m_freeCf = m_freeCf + m_cfAllocation[id][ueTask.m_taskId];
    itUe->second.erase(itTask);

    NS_LOG_DEBUG("The free Cf of CfUnit " << m_id << " is " << m_freeCf.m_cfCapacity << " TFLOPS"
                                          << std::endl);
}

void
CfUnit::EndTask(uint64_t id, UeTaskModel ueTask)
{
    NS_LOG_FUNCTION(this << id << ueTask.m_taskId);
    DynamicCast<CfApplication>(ueTask.m_application)->RecvTaskResult(id, ueTask);
    // NS_LOG_DEBUG("UE " << id << " Task " << ueTask.m_taskId << " end");
    FreeCf(id, ueTask);
    DeleteUeTask(id, ueTask);

    OutputCfAllocationInfo();
    // TODO interact with app
}

void
CfUnit::OutputCfAllocationInfo()
{
    uint tab = 20;
    std::ostringstream oss;
    oss << std::setw(tab) << "ueId" << std::setw(tab) << "taskId" << std::setw(tab) << "cfLoad"
        << std::setw(tab) << "cfAllocated" << std::endl;

    for (auto itUe = m_cfAllocation.begin(); itUe != m_cfAllocation.end(); itUe++)
    {
        auto ueId = itUe->first;
        for (auto itTask = itUe->second.begin(); itTask != itUe->second.end(); itTask++)
        {
            auto taskId = itTask->first;
            auto cfAllocated = m_cfAllocation[ueId][taskId].m_cfCapacity;
            auto cfLoad = m_ueTask[ueId][taskId].m_cfLoad;
            oss << std::setw(tab) << ueId << std::setw(tab) << taskId << std::setw(tab) << cfLoad
                << std::setw(tab) << cfAllocated << std::endl;
        }
    }
    NS_LOG_DEBUG(oss.str());
}

CfModel
CfUnit::GetCf()
{
    return m_cf;
}
} // namespace ns3
