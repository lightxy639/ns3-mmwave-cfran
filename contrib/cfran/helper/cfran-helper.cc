#include "cfran-helper.h"

#include <ns3/cf-application.h>
#include <ns3/config.h>
#include <ns3/object-factory.h>
#include <ns3/ue-cf-application.h>

namespace ns3
{

/* ... */
NS_LOG_COMPONENT_DEFINE("CfRanHelper");

NS_OBJECT_ENSURE_REGISTERED(CfRanHelper);

CfRanHelper::CfRanHelper()
{
    NS_LOG_FUNCTION(this);

    m_cfE2eCalculator = CreateObject<CfE2eCalculator>();
    // m_cfTimeBuffer = CreateObject<CfE2eBuffer>();
    m_cfTimeBuffer = CreateObject<CfTimeBuffer>();
}

CfRanHelper::~CfRanHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId
CfRanHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::CfRanHelper").SetParent<Object>().AddConstructor<CfRanHelper>();
    return tid;
}

void
CfRanHelper::InstallCfUnit(NodeContainer c, ObjectFactory cfUnitObj)
{
    NS_LOG_FUNCTION(this);

    for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i)
    {
        Ptr<CfUnit> cfUnit = DynamicCast<CfUnit>(cfUnitObj.Create());
        (*i)->AggregateObject(cfUnit);
    }
}

void
CfRanHelper::EnableTraces(ApplicationContainer ueAppC, ApplicationContainer gnbAppC)
{
    NS_LOG_FUNCTION(this);
    // This statement will cause the TraceSource function of CfApplicaiton to be unavailable,
    // implying that it will not be investigated further
    // Config::ConnectWithoutContextFailSafe(
    //     "/NodeList/*/$ns3::CfUnit/ComputingTask",
    //     MakeBoundCallback(&CfE2eBuffer::CfUnitStartCompCallback, m_cfTimeBuffer));

    Config::ConnectWithoutContextFailSafe(
        "/NodeList/*/ApplicationList/*/SendRequest",
        MakeBoundCallback(&CfTimeBuffer::CfTimeBufferCallback, m_cfTimeBuffer));

    Config::ConnectWithoutContextFailSafe(
        "/NodeList/*/ApplicationList/*/RecvResult",
        MakeBoundCallback(&CfTimeBuffer::CfTimeBufferCallback, m_cfTimeBuffer));

    Config::ConnectWithoutContextFailSafe(
        "/NodeList/*/ApplicationList/*/RecvRequest",
        MakeBoundCallback(&CfTimeBuffer::CfTimeBufferCallback, m_cfTimeBuffer));

    Config::ConnectWithoutContextFailSafe(
        "/NodeList/*/ApplicationList/*/SendResult",
        MakeBoundCallback(&CfTimeBuffer::CfTimeBufferCallback, m_cfTimeBuffer));

    Config::ConnectWithoutContextFailSafe(
        "/NodeList/*/ApplicationList/*/RecvRequestToBeForwarded",
        MakeBoundCallback(&CfTimeBuffer::CfTimeBufferCallback, m_cfTimeBuffer));

    Config::ConnectWithoutContextFailSafe(
        "/NodeList/*/ApplicationList/*/RecvForwardedResult",
        MakeBoundCallback(&CfTimeBuffer::CfTimeBufferCallback, m_cfTimeBuffer));

    Config::ConnectWithoutContextFailSafe(
        "/NodeList/*/ApplicationList/*/AddTask",
        MakeBoundCallback(&CfTimeBuffer::CfTimeBufferCallback, m_cfTimeBuffer));

    Config::ConnectWithoutContext(
        "/NodeList/*/ApplicationList/*/CfUnit/ProcessTask",
        MakeBoundCallback(&CfTimeBuffer::CfTimeBufferCallback, m_cfTimeBuffer));

    // Config::SetDefault("ns3::UeCfApplication::CfE2eBuffer", PointerValue(m_cfTimeBuffer));
    // Config::SetDefault("ns3::UeCfApplication::CfE2eCalculator", PointerValue(m_cfE2eCalculator));

    for (uint32_t n = 0; n < ueAppC.GetN(); n++)
    {
        Ptr<UeCfApplication> ueApp = DynamicCast<UeCfApplication>(ueAppC.Get(n));
        ueApp->SetAttribute("CfTimeBuffer", PointerValue(m_cfTimeBuffer));
        ueApp->SetAttribute("CfE2eCalculator", PointerValue(m_cfE2eCalculator));
    }

    for (uint32_t n = 0; n < gnbAppC.GetN(); n++)
    {
        Ptr<CfApplication> gnbApp = DynamicCast<CfApplication>(gnbAppC.Get(n));
        gnbApp->SetAttribute("CfE2eCalculator", PointerValue(m_cfE2eCalculator));
    }
}

} // namespace ns3
