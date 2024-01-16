#include "cf-unit.h"

#include <ns3/boolean.h>
#include <ns3/log.h>
#include <ns3/node.h>
#include <ns3/simulator.h>

#include <iostream>
#include <iomanip>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("CfUnit");

NS_OBJECT_ENSURE_REGISTERED(CfUnit);

TypeId
CfUnit::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3:CfUnit")
            .SetParent<Object>()
            .AddConstructor<CfUnit>()
            .AddAttribute("EnableAutoSchedule",
                          "If true,  CfUnit allocate computing force for tasks autonomously."
                          "Otherwise, CfUnit will obey the instructions of the application.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&CfUnit::m_enableAutoSchedule),
                          MakeBooleanChecker());

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
CfUnit::AddNewUeTaskForSchedule(uint16_t id, UeTaskModel ueTask)
{
    NS_LOG_FUNCTION(this << id << ueTask.m_taskId);
    auto it = m_ueTask.find(id);
    if (it != m_ueTask.end())
    {
        NS_LOG_DEBUG("Add new task " << ueTask.m_taskId << " for the task vector of UE " << id
                                     << std::endl);

        (it->second).push_back(ueTask);
    }
    else
    {
        NS_LOG_DEBUG("Create new task vector for UE " << id << std::endl);
        std::vector<UeTaskModel> ueTaskVector;
        ueTaskVector.push_back(ueTask);

        NS_LOG_DEBUG("Add new task " << ueTask.m_taskId << " for the task vector of UE " << id
                                     << std::endl);
        m_ueTask.insert(std::pair<uint16_t, std::vector<UeTaskModel>>(id, ueTaskVector));
    }

    NS_LOG_DEBUG("m_enableAutoSchedule " << m_enableAutoSchedule);
    if (!m_enableAutoSchedule)
    {
        ScheduleNewTaskWithCommand(id, ueTask);
    }
}

void
CfUnit::DeleteUeTask(uint16_t id, UeTaskModel ueTask)
{
    NS_LOG_FUNCTION(this << id << ueTask.m_taskId);

    auto it = m_ueTask.find(id);
    NS_ASSERT(it != m_ueTask.end());

    auto itTask = find(it->second.begin(), it->second.end(), ueTask);
    NS_ASSERT(itTask != it->second.end());

    it->second.erase(itTask);
}

void
CfUnit::AllocateCf(uint16_t id, UeTaskModel ueTask, CfModel cfModel)
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
CfUnit::ScheduleNewTaskWithCommand(uint16_t id, UeTaskModel ueTask)
{
    NS_LOG_FUNCTION(this << id << ueTask.m_taskId);
    NS_ASSERT(m_freeCf >= ueTask.m_cfRequired);

    AllocateCf(id, ueTask, ueTask.m_cfRequired);

    ExecuteTask(id, ueTask);
}

void
CfUnit::ExecuteTask(uint16_t id, UeTaskModel ueTask)
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
CfUnit::FreeCf(uint16_t id, UeTaskModel ueTask)
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
CfUnit::EndTask(uint16_t id, UeTaskModel ueTask)
{
    NS_LOG_DEBUG("UE " << id << " Task " << ueTask.m_taskId << " end");
    FreeCf(id, ueTask);
    // TODO interact with app
}

void
CfUnit::OutputCfAllocationInfo()
{
    std::ostringstream oss;
    oss << std::endl << std::setfill(' ') << std::endl;
}

CfModel
CfUnit::GetCf()
{
    return m_cf;
}
} // namespace ns3