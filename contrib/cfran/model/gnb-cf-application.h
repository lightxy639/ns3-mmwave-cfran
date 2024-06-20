#ifndef GNB_CF_APPLICATION_H
#define GNB_CF_APPLICATION_H

#include <ns3/cf-application.h>
#include <ns3/cf-e2e-calculator.h>
#include <ns3/cf-model.h>
#include <ns3/cf-unit.h>
#include <ns3/epc-x2.h>
#include <ns3/multi-packet-header.h>
#include <ns3/multi-packet-manager.h>
#include <ns3/packet.h>
#include <ns3/socket.h>
#include <ns3/system-info.h>

namespace ns3
{

// struct CfModel;
// struct UeTaskModel;
// class CfUnit;

/**
 * CfX2IfaceInfo
 */
class CfX2IfaceInfo : public SimpleRefCount<CfX2IfaceInfo>
{
  public:
    /**
     * Constructor
     *
     * \param remoteIpAddr remote IP address
     * \param localCtrlPlaneSocket control plane socket
     * \param localUserPlaneSocket user plane socket
     */
    CfX2IfaceInfo(Ipv4Address remoteIpAddr, Ptr<Socket> localSocket);
    virtual ~CfX2IfaceInfo(void);

    /**
     * Assignment operator
     *
     * \returns CfX2IfaceInfo&
     */
    CfX2IfaceInfo& operator=(const CfX2IfaceInfo&);

  public:
    Ipv4Address m_remoteIpAddr; ///< remote IP address
    Ptr<Socket> m_localSocket;  ///< local control plane socket
};

/**
 * \brief The implement of applications that require computing force
 */
class GnbCfApplication : public CfApplication
{
  public:
    // enum UeState
    // {
    //     Initializing = 0,
    //     Serving = 1,
    //     Migrating = 2,
    //     Over = 3
    // };
    enum PolicyMode
    {
      Local = 0, // Process locally
      E2 = 1 // Report to RIC and wait for instruction
    };
    struct Policy
    {
      uint64_t m_ueId;
      uint64_t m_offloadPointId;
    };

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    GnbCfApplication();

    ~GnbCfApplication() override;

    // void SetCfUnit(Ptr<CfUnit> cfUnit);

    void SetMmWaveEnbNetDevice(Ptr<mmwave::MmWaveEnbNetDevice> mmwaveEnbNetDev);

    virtual void SetClientFd(int clientFd) override;

    void InitX2Info();

    void RecvTaskRequest();

    void MigrateUeService(uint64_t ueId, uint64_t targetGnbId);

    void SendPacketToUe(uint64_t ueId, Ptr<Packet> packet) override;
    void RecvFromUe(Ptr<Socket> socket) override;
    // virtual

    // void LoadTaskToCfUnit(uint64_t id, UeTaskModel ueTask) override;

    void RecvTaskResult(uint64_t id, UeTaskModel ueTask) override;

    // virtual void RecvExecutingInform(uint64_t id, UeTaskModel ueTask);

    virtual void SendInitSuccessToUserFromGnb(uint64_t id);

    virtual void SendInitSuccessToConnectedGnb(uint64_t ueId);

    // If the application is installed on a cloud server, the traditional process is completed
    virtual void SendTaskResultToUserFromRemote(uint64_t id, Ptr<Packet> packet);

  protected:


    Ptr<mmwave::MmWaveEnbNetDevice> m_mmWaveEnbNetDevice;

    uint16_t m_cfX2Port;

    uint64_t m_appSize;

    std::map<uint16_t, Ptr<CfX2IfaceInfo>> m_cfX2InterfaceSockets;

    // std::map<uint64_t, UeState> m_ueState;



  private:
    /**
     * \brief Handle a packet reception.
     *
     * This function is called by lower layers.
     *
     * \param socket the socket the packet was received to.
     */

    void SendPacketToOtherGnb(uint64_t gnbId, Ptr<Packet> packet);

    void HandleRead(Ptr<Socket> socket);

    void RecvFromOtherGnb(Ptr<Socket> socket);

    // void UpdateUeState(uint64_t id, UeState state);

    void CompleteMigrationAtTarget(uint64_t ueId, uint64_t oldGnbId);

    void SendNewUeReport(uint64_t ueId);

    void AssignUe(uint64_t ueId, uint64_t offloadPointId);

    void BuildAndSendE2Report();

    void ControlMessageReceivedCallback(E2AP_PDU_t *pdu);

    void ExecuteCommands();

    virtual void StartApplication(); // Called at time specified by Start

    virtual void StopApplication(); // Called at time specified by Stop


    // TracedCallback<uint64_t, uint64_t, uint64_t> m_forwardRequestTrace;

    TracedCallback<uint64_t, uint64_t, uint64_t> m_recvRequestToBeForwardedTrace;
    TracedCallback<uint64_t, uint64_t, uint64_t> m_recvForwardedRequestTrace;

    TracedCallback<uint64_t, uint64_t, uint64_t> m_forwardResultTrace;
    TracedCallback<uint64_t, uint64_t, uint64_t> m_getForwardedResultTrace;

    std::string m_ueE2eOutFile;
    bool m_firstWrite;

    PolicyMode m_policyMode;
    std::queue<Policy> m_policy;
};

} // namespace ns3

#endif
