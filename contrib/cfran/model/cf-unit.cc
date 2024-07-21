#include "cf-unit.h"

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
            .AddAttribute("CfModel",
                          "The Computing force of the CfUnitSimple",
                          CfModelValue(CfModel()),
                          MakeCfModelAccessor(&CfUnit::m_cf),
                          MakeCfModelChecker())
            .AddTraceSource("ProcessTask",
                            "Process UE task",
                            MakeTraceSourceAccessor(&CfUnit::m_processTaskTrace),
                            "ns3::TaskComputing::TracedCallback");

    return tid;
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
    : m_cf(cf)
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
CfUnit::SetCfUnitId(uint64_t id)
{
    m_id = id;
}

void
CfUnit::SetCfApplication(Ptr<CfApplication> cfApp)
{
    m_cfApplication = cfApp;
}

CfModel
CfUnit::GetCf()
{
    return m_cf;
}

} // namespace ns3