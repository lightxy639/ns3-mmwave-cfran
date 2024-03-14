#include "cf-unit.h"

#include <ns3/boolean.h>
#include <ns3/log.h>
#include <ns3/node.h>
#include <ns3/simulator.h>

#include <iomanip>
#include <iostream>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("CfUnitSimple");

NS_OBJECT_ENSURE_REGISTERED(CfUnitSimple);

TypeId
CfUnitSimple::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::CfUnitSimple")
            .SetParent<Object>()
            .AddConstructor<CfUnitSimple>()
            .AddAttribute("EnableAutoSchedule",
                          "If true,  CfUnitSimple allocate computing force for tasks autonomously."
                          "Otherwise, CfUnitSimple will obey the instructions of the application.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&CfUnitSimple::m_enableAutoSchedule),
                          MakeBooleanChecker())
            .AddAttribute("CfModel",
                          "The Computing force of the CfUnitSimple",
                          CfModelValue(CfModel()),
                          MakeCfModelAccessor(&CfUnitSimple::m_cf),
                          MakeCfModelChecker());

    return tid;
}

void
CfUnitSimple::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    Object::DoInitialize();
}

void
CfUnitSimple::DoDispose()
{
    NS_LOG_FUNCTION(this);
    Object::DoDispose();
}

CfUnitSimple::CfUnitSimple()
{
    NS_LOG_FUNCTION(this);
    CfModel cf("GPU", 82.6);
    m_cf = cf;
}

CfUnitSimple::CfUnitSimple(CfModel cf)
    : m_cf(cf),
      m_freeCf(cf)
{
    NS_LOG_FUNCTION(this);
}

CfUnitSimple::~CfUnitSimple()
{
    NS_LOG_FUNCTION(this);
}

void
CfUnitSimple::SetNode(Ptr<Node> node)
{
    NS_LOG_FUNCTION(this << node);
    m_node = node;
}

void
CfUnitSimple::ScheduleTasks()
{
    NS_LOG_FUNCTION(this);
}

void
CfUnitSimple::AddNewUeTaskForSchedule(uint64_t id, UeTaskModel ueTask)
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
CfUnitSimple::DeleteUeTask(uint64_t id, UeTaskModel ueTask)
{
    NS_LOG_FUNCTION(this << id << ueTask.m_taskId);

    auto itUe = m_ueTask.find(id);
    NS_ASSERT(itUe != m_ueTask.end());

    auto itTask = itUe->second.find(ueTask.m_taskId);
    NS_ASSERT(itTask != itUe->second.end());

    itUe->second.erase(itTask);
}

void
CfUnitSimple::AllocateCf(uint64_t id, UeTaskModel ueTask, CfModel cfModel)
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

    NS_LOG_DEBUG("The free Cf of CfUnitSimple " << m_id << " is " << m_freeCf.m_cfCapacity << " TFLOPS"
                                          << std::endl);
}

void
CfUnitSimple::ScheduleNewTaskWithCommand(uint64_t id, UeTaskModel ueTask)
{
    NS_LOG_FUNCTION(this << id << ueTask.m_taskId);
    NS_ASSERT(m_freeCf >= ueTask.m_cfRequired);

    AllocateCf(id, ueTask, ueTask.m_cfRequired);

    ExecuteTask(id, ueTask);

    OutputCfAllocationInfo();
}

void
CfUnitSimple::ExecuteTask(uint64_t id, UeTaskModel ueTask)
{
    NS_LOG_FUNCTION(this << id << ueTask.m_taskId);

    CfModel allocateCf = m_cfAllocation[id][ueTask.m_taskId];
    float allocatedCapacity = allocateCf.m_cfCapacity;

    float computingLatency = ueTask.m_cfLoad / allocatedCapacity * 1000; // ms

    NS_LOG_DEBUG("UE " << id << " task " << ueTask.m_taskId << " computing latency "
                       << computingLatency << "ms");

    Simulator::Schedule(MilliSeconds(computingLatency), &CfUnitSimple::EndTask, this, id, ueTask);
}

void
CfUnitSimple::FreeCf(uint64_t id, UeTaskModel ueTask)
{
    NS_LOG_FUNCTION(this << id << ueTask.m_taskId);

    auto itUe = m_cfAllocation.find(id);
    NS_ASSERT(itUe != m_cfAllocation.end());

    auto itTask = itUe->second.find(ueTask.m_taskId);

    NS_ASSERT(itTask != itUe->second.end());

    // Delete the info in allocation map and calculator free computing force
    m_freeCf = m_freeCf + m_cfAllocation[id][ueTask.m_taskId];
    itUe->second.erase(itTask);

    NS_LOG_DEBUG("The free Cf of CfUnitSimple " << m_id << " is " << m_freeCf.m_cfCapacity << " TFLOPS"
                                          << std::endl);
}

void
CfUnitSimple::EndTask(uint64_t id, UeTaskModel ueTask)
{
    NS_LOG_FUNCTION(this << id << ueTask.m_taskId);
    DynamicCast<CfApplication>(ueTask.m_application)->RecvTaskResult(id, ueTask);
    // NS_LOG_DEBUG("UE " << id << " Task " << ueTask.m_taskId << " end");
    FreeCf(id, ueTask);
    DeleteUeTask(id, ueTask);

    OutputCfAllocationInfo();
}

void
CfUnitSimple::OutputCfAllocationInfo()
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
CfUnitSimple::GetCf()
{
    return m_cf;
}
} // namespace ns3
