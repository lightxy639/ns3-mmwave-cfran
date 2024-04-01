#include "cfran-helper.h"

#include <ns3/config.h>
#include <ns3/object-factory.h>

namespace ns3
{

/* ... */
NS_LOG_COMPONENT_DEFINE("CfRanHelper");

NS_OBJECT_ENSURE_REGISTERED(CfRanHelper);

CfRanHelper::CfRanHelper()
{
    NS_LOG_FUNCTION(this);

    m_cfE2eCalculator = Create<CfE2eCalculator>();
    m_cfE2eBuffer = Create<CfE2eBuffer>();
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
CfRanHelper::EnableTraces()
{
    NS_LOG_FUNCTION(this);
    // This statement will cause the TraceSource function of CfApplicaiton to be unavailable,
    // implying that it will not be investigated further
    // Config::ConnectWithoutContextFailSafe(
    //     "/NodeList/*/$ns3::CfUnit/ComputingTask",
    //     MakeBoundCallback(&CfE2eBuffer::CfUnitStartCompCallback, m_cfE2eBuffer));

    Config::ConnectWithoutContextFailSafe(
        "/NodeList/*/ApplicationList/*/TxRequest",
        MakeBoundCallback(&CfE2eBuffer::UeSendRequestCallback, m_cfE2eBuffer));

    Config::ConnectWithoutContextFailSafe(
        "/NodeList/*/ApplicationList/*/RxResult",
        MakeBoundCallback(&CfE2eBuffer::UeRecvResultCallback, m_cfE2eBuffer));

    Config::ConnectWithoutContextFailSafe(
        "/NodeList/*/ApplicationList/*/RecvRequest",
        MakeBoundCallback(&CfE2eBuffer::GnbRecvUeRequestCallback, m_cfE2eBuffer));
    Config::ConnectWithoutContextFailSafe(
        "/NodeList/*/ApplicationList/*/ForwardRequest",
        MakeBoundCallback(&CfE2eBuffer::GnbForwardUeRequestCallback, m_cfE2eBuffer));
    Config::ConnectWithoutContextFailSafe(
        "/NodeList/*/ApplicationList/*/RecvForwardedRequest",
        MakeBoundCallback(&CfE2eBuffer::GnbRecvForwardedUeRequestCallback, m_cfE2eBuffer));
    Config::ConnectWithoutContextFailSafe(
        "/NodeList/*/ApplicationList/*/AddTask",
        MakeBoundCallback(&CfE2eBuffer::CfAppAddTaskCallback, m_cfE2eBuffer));

    Config::ConnectWithoutContext(
        "/NodeList/*/ApplicationList/*/CfUnit/ComputingTask",
        MakeBoundCallback(&CfE2eBuffer::CfUnitStartCompCallback, m_cfE2eBuffer));

    Config::ConnectWithoutContextFailSafe(
        "/NodeList/*/ApplicationList/*/GetResult",
        MakeBoundCallback(&CfE2eBuffer::CfAppGetResultCallback, m_cfE2eBuffer));
    Config::ConnectWithoutContextFailSafe(
        "/NodeList/*/ApplicationList/*/ForwardResult",
        MakeBoundCallback(&CfE2eBuffer::GnbForwardResultCallback, m_cfE2eBuffer));
    Config::ConnectWithoutContextFailSafe(
        "/NodeList/*/ApplicationList/*/GetForwardedResult",
        MakeBoundCallback(&CfE2eBuffer::GnbGetForwardedResultCallback, m_cfE2eBuffer));
    Config::ConnectWithoutContextFailSafe(
        "/NodeList/*/ApplicationList/*/SendResult",
        MakeBoundCallback(&CfE2eBuffer::GnbSendResultToUeCallback, m_cfE2eBuffer));

    Config::SetDefault("ns3::UeCfApplication::CfE2eBuffer", PointerValue(m_cfE2eBuffer));
    Config::SetDefault("ns3::UeCfApplication::CfE2eCalculator", PointerValue(m_cfE2eCalculator));
}

} // namespace ns3
