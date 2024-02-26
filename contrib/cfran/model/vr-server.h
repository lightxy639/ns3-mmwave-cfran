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

    /**
     * \brief Start VR service for single user
     * \param imsi the imsi of user
     */
    void StartServiceForImsi(uint64_t imsi);

    /**
     * \brief Stop VR service for single user
     * \param imsi the imsi of user
     */
    void StopServiceForImsi(uint64_t imsi);

    /**
     * \brief Build and send the RIC Indication Message about VR server
     * // TODO
     */
    void BuildAndSendReportVrApplicationMsg();

    /**
     * \brief Record information of single new user.
     * \param imsi the imsi of user
     * \param info UeApplicationInfo struct
     */
    void AddApplicationInfoForImsi(uint64_t imsi, UeApplicationInfo info);

    /**
     * \brief Delete information of single user who has already left
     * \param imsi the imsi of user
     */
    void DeleteApplicationInfoForImsi(uint64_t imsi);

    /**
     * \brief Load a specific task to a computing unit
     * \param id the id of user
     * \param ueTask the structure that stores task information
     */
    void LoadTaskToCfUnit(uint64_t id, UeTaskModel ueTask) override;

    /**
     * \brief Receive the result of specific task from the computing uint
     * \param id the id of user
     * \param ueTask the structure that stores task information
     */
    void RecvTaskResult(uint64_t id, UeTaskModel ueTask) override;

  protected:
    void DoDispose() override;

  private:
    // inherited from Application base class.
    void StartApplication() override; // Called at time specified by Start

    void StopApplication() override; // Called at time specified by Stop

    /**
     * \brief Handle the rendering task of a single user
     * \param imsi the imsi of user
     */
    void Handle4Imsi(uint64_t imsi);

    /**
     * \brief Send a frame to user using mmwaveEnbNetDevice if this app is
     *        installed on a gNB node
     * \param imsi the imsi of user
     */
    void Send2ImsiFromGnb(uint64_t imsi);

    /**
     * \brief Send a frame to user using socket if this app is
     *        installed on a remote host
     * \param imsi the imsi of user
     */
    void Send2ImsiFromRemoteHost(uint64_t imsi);

    Ptr<CfranSystemInfo> m_cfranSystemInfo;

    std::map<uint64_t, UeApplicationInfo> m_ueApplicationInfo;
};

} // namespace ns3

#endif
