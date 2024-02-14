#include "cf-model.h"

// #include <ns3/cf-application.h>
#include <ns3/assert.h>
#include <ns3/double.h>
#include <ns3/log.h>
#include <ns3/string.h>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("CfModel");

ATTRIBUTE_HELPER_CPP(CfModel);

std::ostream&
operator<<(std::ostream& os, const CfModel& cfModel)
{
    os << cfModel.m_cfType << "|" << cfModel.m_cfCapacity;
    return os;
}

std::istream&
operator>>(std::istream& is, CfModel& cfModel)
{
    char c1, c2;
    is >> cfModel.m_cfType >> c1 >> cfModel.m_cfCapacity >> c2;
    if (c1 != '|' || c2 != '|')
    {
        is.setstate(std::ios_base::failbit);
    }
    return is;
}

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

// TypeId
// CfModel::GetTypeId()
// {
//     static TypeId tid = TypeId("ns3::CfModel")
//                             .SetParent<Object>()
//                             .AddConstructor<CfModel>()
//                             .AddAttribute("CfType",
//                                           "The type of computing force.",
//                                           StringValue("GPU"),
//                                           MakeStringAccessor(&CfModel::m_cfType),
//                                           MakeStringChecker())
//                             .AddAttribute("CfCapacity",
//                                           "The capacity value of the cfunit.",
//                                           DoubleValue(82.6),
//                                           MakeDoubleAccessor(&CfModel::m_cfCapacity),
//                                           MakeDoubleChecker<double>());

//     return tid;
// }

// CfModel::~CfModel()
// {
//     NS_LOG_FUNCTION(this);
// }

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
CfModel::operator/(uint64_t num)
{
    NS_ASSERT(num != 0);
    CfModel result(m_cfType, m_cfCapacity / num);

    return result;
}

UeTaskModel::UeTaskModel()
    : m_taskId(0),
      m_cfRequired(CfModel("GPU", 0)),
      m_cfLoad(0),
      m_deadline(0)
{
}

UeTaskModel::UeTaskModel(uint64_t taskId, CfModel cfRequired, float cfLoad, float deadline)
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
