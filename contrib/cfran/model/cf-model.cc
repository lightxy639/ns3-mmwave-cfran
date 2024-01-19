#include "cf-model.h"

#include <ns3/assert.h>
#include <ns3/log.h>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("CfModel");

CfModel::CfModel()
    : m_cfType("GPU"),
      m_cfCapacity(82.6)
{
}

CfModel::CfModel(std::string cfType, float cfCapacity)
    : m_cfType(cfType),
      m_cfCapacity(cfCapacity)
{
}

CfModel
CfModel::operator+(const CfModel& param)
{
    NS_ASSERT(m_cfType == param.m_cfType);
    CfModel temp(m_cfType, 0);
    temp.m_cfCapacity = m_cfCapacity + param.m_cfCapacity;

    return temp;
}

CfModel
CfModel::operator-(const CfModel& param)
{
    NS_ASSERT(m_cfType == param.m_cfType);
    CfModel temp(m_cfType, 0);
    temp.m_cfCapacity = m_cfCapacity - param.m_cfCapacity;

    return temp;
}

bool
CfModel::operator>=(const CfModel& param)
{
    NS_ASSERT(m_cfType == param.m_cfType);

    return m_cfCapacity >= param.m_cfCapacity;
}

CfModel
CfModel::operator/ (uint16_t num)
{
    NS_ASSERT(num != 0);
    CfModel result(m_cfType, m_cfCapacity/num);

    return result;
}


UeTaskModel::UeTaskModel() : m_taskId(0), m_cfRequired(CfModel("GPU", 0)),m_cfLoad(0), m_deadline(0)
{
}

UeTaskModel::UeTaskModel(uint16_t taskId, CfModel cfRequired, float cfLoad, float deadline)
    : m_taskId(taskId),
      m_cfRequired(cfRequired),
      m_cfLoad(cfLoad),
      m_deadline(deadline)
{
}

bool
UeTaskModel::operator==(const UeTaskModel& param)
{
    return m_taskId == param.m_taskId;
}
} // namespace ns3