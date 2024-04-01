#include "cf-e2e-buffer.h"

#include <ns3/config.h>
#include <ns3/log.h>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("CfE2eBuffer");
NS_OBJECT_ENSURE_REGISTERED(CfE2eBuffer);

CfE2eBuffer::CfE2eBuffer()
{
    NS_LOG_FUNCTION(this);
}

TypeId
CfE2eBuffer::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::CfE2eBuffer").SetParent<Object>().AddConstructor<CfE2eBuffer>();
    return tid;
}

void
CfE2eBuffer::DoDispose()
{
    NS_LOG_FUNCTION(this);
}

CfE2eBuffer::~CfE2eBuffer()
{
    NS_LOG_FUNCTION(this);
}

uint64_t
CfE2eBuffer::GetSpecificDelayBuffer(uint64_t ueId,
                                    uint64_t taskId,
                                    DetailedDelayStatsBuffer& delayBuffer,
                                    bool erase)
{
    UeTaskIdPair_t p(ueId, taskId);
    auto it = delayBuffer.find(p);

    if (it != delayBuffer.end())
    {
        uint64_t delay = it->second;
        if (erase)
        {
            delayBuffer.erase(it);
        }
        return delay;
    }
    else
    {
        // NS_FATAL_ERROR("Detailed delay for " << ueId << " not found");
        NS_LOG_DEBUG("Value don't exist, return 0");
        return 0;
    }
}

void
CfE2eBuffer::UpdateSpecificDelayBuffer(uint64_t ueId,
                                       uint64_t taskId,
                                       uint64_t delay,
                                       DetailedDelayStatsBuffer& delayBuffer)
{
    UeTaskIdPair_t p(ueId, taskId);
    auto it = delayBuffer.find(p);

    if (it == delayBuffer.end())
    {
        delayBuffer[p] = delay;
    }
    else
    {
        // Performing latency calculation at the same address.
        delayBuffer[p] = delay;
        NS_LOG_DEBUG("Update ");
    }
}

void
CfE2eBuffer::UpdateUplinkWirelessDelay(uint64_t ueId, uint64_t taskId, uint64_t delay)
{
    UpdateSpecificDelayBuffer(ueId, taskId, delay, m_uplinkWirelessDelay);
}

uint64_t
CfE2eBuffer::GetUplinkWirelessDelay(uint64_t ueId, uint64_t taskId, bool erase)
{
    return GetSpecificDelayBuffer(ueId, taskId, m_uplinkWirelessDelay, erase);
}

void
CfE2eBuffer::UpdateUplinkWiredDelay(uint64_t ueId, uint64_t taskId, uint64_t delay)
{
    UpdateSpecificDelayBuffer(ueId, taskId, delay, m_uplinkWiredDelay);
}

uint64_t
CfE2eBuffer::GetUplinkWiredDelay(uint64_t ueId, uint64_t taskId, bool erase)
{
    return GetSpecificDelayBuffer(ueId, taskId, m_uplinkWiredDelay, erase);
}

void
CfE2eBuffer::UpdateQueueDelay(uint64_t ueId, uint64_t taskId, uint64_t delay)
{
    UpdateSpecificDelayBuffer(ueId, taskId, delay, m_queueDelay);
}

uint64_t
CfE2eBuffer::GetQueueDelay(uint64_t ueId, uint64_t taskId, bool erase)
{
    return GetSpecificDelayBuffer(ueId, taskId, m_queueDelay, erase);
}

void
CfE2eBuffer::UpdateComputingDelay(uint64_t ueId, uint64_t taskId, uint64_t delay)
{
    UpdateSpecificDelayBuffer(ueId, taskId, delay, m_computingDelay);
}

uint64_t
CfE2eBuffer::GetComputingDelay(uint64_t ueId, uint64_t taskId, bool erase)
{
    return GetSpecificDelayBuffer(ueId, taskId, m_computingDelay, erase);
}

void
CfE2eBuffer::UpdateDownlinkWiredDelay(uint64_t ueId, uint64_t taskId, uint64_t delay)
{
    UpdateSpecificDelayBuffer(ueId, taskId, delay, m_downlinkWiredDelay);
}

uint64_t
CfE2eBuffer::GetDownlinkWiredDelay(uint64_t ueId, uint64_t taskId, bool erase)
{
    return GetSpecificDelayBuffer(ueId, taskId, m_downlinkWiredDelay, erase);
}

void
CfE2eBuffer::UpdateDownlinkWirelessDelay(uint64_t ueId, uint64_t taskId, uint64_t delay)
{
    UpdateSpecificDelayBuffer(ueId, taskId, delay, m_downlinkWirelessDelay);
}

uint64_t
CfE2eBuffer::GetDownlinkWirelessDelay(uint64_t ueId, uint64_t taskId, bool erase)
{
    return GetSpecificDelayBuffer(ueId, taskId, m_downlinkWirelessDelay, erase);
}

void
CfE2eBuffer::UeSendRequestCallback(Ptr<CfE2eBuffer> stats,
                                   uint64_t ueId,
                                   uint64_t taskId,
                                   uint64_t delay)
{
    NS_LOG_DEBUG("CfE2eBuffer::UeSendRequestCallback");
    stats->UpdateUplinkWirelessDelay(ueId, taskId, delay);
}

void
CfE2eBuffer::GnbRecvUeRequestCallback(Ptr<CfE2eBuffer> stats,
                                      uint64_t ueId,
                                      uint64_t taskId,
                                      uint64_t delay)
{
    NS_LOG_DEBUG("CfE2eBuffer::GnbRecvUeRequestCallback");
    uint64_t temp = stats->GetUplinkWirelessDelay(ueId, taskId);
    if (temp != 0)
    {
        stats->UpdateUplinkWirelessDelay(ueId, taskId, delay - temp);
        NS_LOG_DEBUG("(ueId, taskId) " << ueId << ", " << taskId << " upWl " << (delay - temp) / 1e6
                                       << "ms");
    }
    else
    {
        NS_FATAL_ERROR("No temp value found");
    }
}

void
CfE2eBuffer::GnbForwardUeRequestCallback(Ptr<CfE2eBuffer> stats,
                                         uint64_t ueId,
                                         uint64_t taskId,
                                         uint64_t delay)
{
    NS_LOG_DEBUG("CfE2eBuffer::GnbForwardUeRequestCallback");
    stats->UpdateUplinkWiredDelay(ueId, taskId, delay);
}

void
CfE2eBuffer::GnbRecvForwardedUeRequestCallback(Ptr<CfE2eBuffer> stats,
                                               uint64_t ueId,
                                               uint64_t taskId,
                                               uint64_t delay)
{
    NS_LOG_DEBUG("CfE2eBuffer::GnbRecvForwardedUeRequestCallback");
    uint64_t temp = stats->GetUplinkWiredDelay(ueId, taskId);
    if (temp != 0)
    {
        stats->UpdateUplinkWiredDelay(ueId, taskId, delay - temp);
        NS_LOG_DEBUG("(ueId, taskId) " << ueId << ", " << taskId << " upWd " << (delay - temp) / 1e6
                                       << "ms");
    }
    else
    {
        NS_FATAL_ERROR("No temp value found");
    }
}

void
CfE2eBuffer::CfAppAddTaskCallback(Ptr<CfE2eBuffer> stats,
                                  uint64_t ueId,
                                  uint64_t taskId,
                                  uint64_t delay)
{
    NS_LOG_DEBUG("CfE2eBuffer::CfAppAddTaskCallback");
    stats->UpdateQueueDelay(ueId, taskId, delay);
}

void
CfE2eBuffer::CfUnitStartCompCallback(Ptr<CfE2eBuffer> stats,
                                     uint64_t ueId,
                                     uint64_t taskId,
                                     uint64_t delay)
{
    NS_LOG_DEBUG("CfE2eBuffer::CfUnitStartCompCallback");
    uint64_t temp = stats->GetQueueDelay(ueId, taskId);
    if (temp != 0)
    {
        stats->UpdateQueueDelay(ueId, taskId, delay - temp);
        NS_LOG_DEBUG("(ueId, taskId) " << ueId << ", " << taskId << " queue "
                                       << (delay - temp) / 1e6 << "ms");
    }
    else
    {
        NS_FATAL_ERROR("No temp value found");
    }

    stats->UpdateComputingDelay(ueId, taskId, delay);
}

void
CfE2eBuffer::CfAppGetResultCallback(Ptr<CfE2eBuffer> stats,
                                    uint64_t ueId,
                                    uint64_t taskId,
                                    uint64_t delay)
{
    NS_LOG_DEBUG("CfE2eBuffer::CfAppGetResultCallback");
    uint64_t temp = stats->GetComputingDelay(ueId, taskId);
    if (temp != 0)
    {
        stats->UpdateComputingDelay(ueId, taskId, delay - temp);
        NS_LOG_DEBUG("(ueId, taskId) " << ueId << ", " << taskId << " comp " << (delay - temp) / 1e6
                                       << "ms");
    }
    else
    {
        NS_FATAL_ERROR("No temp value found");
    }
}

void
CfE2eBuffer::GnbForwardResultCallback(Ptr<CfE2eBuffer> stats,
                                      uint64_t ueId,
                                      uint64_t taskId,
                                      uint64_t delay)
{
    NS_LOG_DEBUG("CfE2eBuffer::GnbForwardResultCallback");
    stats->UpdateDownlinkWiredDelay(ueId, taskId, delay);
}

void
CfE2eBuffer::GnbGetForwardedResultCallback(Ptr<CfE2eBuffer> stats,
                                           uint64_t ueId,
                                           uint64_t taskId,
                                           uint64_t delay)
{
    NS_LOG_DEBUG("CfE2eBuffer::GnbGetForwardedResultCallback");
    uint64_t temp = stats->GetDownlinkWiredDelay(ueId, taskId);
    if (temp != 0)
    {
        stats->UpdateDownlinkWiredDelay(ueId, taskId, delay - temp);
        NS_LOG_DEBUG("(ueId, taskId) " << ueId << ", " << taskId << " dnWd " << (delay - temp) / 1e6
                                       << "ms");
    }
    else
    {
        NS_FATAL_ERROR("No temp value found");
    }
}

void
CfE2eBuffer::GnbSendResultToUeCallback(Ptr<CfE2eBuffer> stats,
                                       uint64_t ueId,
                                       uint64_t taskId,
                                       uint64_t delay)
{
    NS_LOG_DEBUG("CfE2eBuffer::GnbSendResultToUeCallback" << " ueId " << ueId << " taskId" << taskId <<" delay " << delay);
    stats->UpdateDownlinkWirelessDelay(ueId, taskId, delay);
}

void
CfE2eBuffer::UeRecvResultCallback(Ptr<CfE2eBuffer> stats,
                                  uint64_t ueId,
                                  uint64_t taskId,
                                  uint64_t delay)
{
    NS_LOG_DEBUG("CfE2eBuffer::UeRecvResultCallback");
    uint64_t temp = stats->GetDownlinkWirelessDelay(ueId, taskId);
    if (temp != 0)
    {
        stats->UpdateDownlinkWirelessDelay(ueId, taskId, delay - temp);
        NS_LOG_DEBUG("(ueId, taskId) " << ueId << ", " << taskId << " dnWl " << (delay - temp) / 1e6
                                       << "ms");
    }
    else
    {
        NS_FATAL_ERROR("No temp value found");
    }
}

} // namespace ns3