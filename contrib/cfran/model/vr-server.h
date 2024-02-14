#ifndef VR_SERVER_H
#define VR_SERVER_H

#include <ns3/application.h>
#include <ns3/cf-application.h>
#include <ns3/cf-model.h>
#include <ns3/cf-unit.h>
#include <ns3/event-id.h>
#include <ns3/system-info.h>

namespace ns3
{
/**
 * \ingroup cfran
 * The VR server implementation
 */

class VrServer : public CfApplication
{
    struct UeApplicationInfo
    {
        UeApplicationInfo(EventId eventId, uint32_t frameHandle, uint32_t frameSent)
            : m_eventId(eventId),
              m_frameHandle(frameHandle),
              m_frameSent(frameSent)
        {
        }

        UeApplicationInfo()
            : m_frameHandle(0),
              m_frameSent(0)
        {
        }

        EventId m_eventId;
        uint32_t m_frameHandle;
        uint32_t m_frameSent;
    };

  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    VrServer();

    ~VrServer();

    void StartServiceForImsi(uint64_t imsi);

    void StopServiceForImsi(uint64_t imsi);

    void BuildAndSendReportVrApplicationMsg();

    void RecvRenderResult();

    void AddApplicationInfoForImsi(uint64_t imsi, UeApplicationInfo info);

    void DeleteApplicationInfoForImsi(uint64_t imsi);

    void LoadTaskToCfUnit(uint64_t id, UeTaskModel ueTask) override;

    void RecvTaskResult(uint64_t id, UeTaskModel ueTask) override;

  protected:
    void DoDispose() override;

  private:
    // inherited from Application base class.
    void StartApplication() override; // Called at time specified by Start

    void StopApplication() override; // Called at time specified by Stop

    void Handle4Imsi(uint64_t imsi);

    void Send2Imsi(uint64_t imsi);

    // Ptr<CfUnit> m_cfUnit;

    Ptr<CfranSystemInfo> m_cfranSystemInfo;

    std::map<uint64_t, UeApplicationInfo> m_ueApplicationInfo;
};

} // namespace ns3

#endif
