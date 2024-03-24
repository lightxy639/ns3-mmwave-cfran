#ifndef CF_APPLICATION_H
#define CF_APPLICATION_H

#include <ns3/application.h>
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

struct CfModel;
struct UeTaskModel;
class CfUnit;

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
class CfApplication : public Application
{
  public:
    enum UeState
    {
        Initializing = 0,
        Serving = 1,
        Migrating = 2,
        Over = 3
    };

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    CfApplication();

    ~CfApplication() override;

    void SetCfUnit(Ptr<CfUnit> cfUnit);

    void SetMmWaveEnbNetDevice(Ptr<mmwave::MmWaveEnbNetDevice> mmwaveEnbNetDev);

    void InitX2Info();

    void RecvTaskRequest();

    void MigrateUeService(uint64_t ueId, uint64_t targetGnbId);
    // virtual

    virtual void LoadTaskToCfUnit(uint64_t id, UeTaskModel ueTask);

    virtual void RecvTaskResult(uint64_t id, UeTaskModel ueTask);

    virtual void SendInitSuccessToUserFromGnb(uint64_t id);

    virtual void SendInitSuccessToConnectedGnb(uint64_t ueId);

    // If the application is installed on a cloud server, the traditional process is completed
    virtual void SendTaskResultToUserFromRemote(uint64_t id, Ptr<Packet> packet);

  protected:
    void DoDispose() override;

    Ptr<CfUnit> m_cfUnit;

    Ptr<mmwave::MmWaveEnbNetDevice> m_mmWaveEnbNetDevice;

    Ptr<CfranSystemInfo> m_cfranSystemInfo;

    Ptr<Socket> m_socket;

    uint16_t m_port;

    uint16_t m_cfX2Port;

    uint64_t m_appSize;

    uint32_t m_defaultPacketSize;

    uint32_t m_initDelay;

    std::map<uint16_t, Ptr<CfX2IfaceInfo>> m_cfX2InterfaceSockets;

    Ptr<MultiPacketManager> m_multiPacketManager;

    std::map<uint64_t, UeState> m_ueState;

    /// Callbacks for tracing the packet Rx events
    TracedCallback<Ptr<const Packet>> m_rxTrace;

    /// Callbacks for tracing the packet Rx events, includes source and destination addresses
    TracedCallback<Ptr<const Packet>, const Address&, const Address&> m_rxTraceWithAddresses;

  private:
    /**
     * \brief Handle a packet reception.
     *
     * This function is called by lower layers.
     *
     * \param socket the socket the packet was received to.
     */

    void SendPakcetToUe(uint64_t ueId, Ptr<Packet> packet);

    void SendPacketToOtherGnb(uint64_t gnbId, Ptr<Packet> packet);

    void HandleRead(Ptr<Socket> socket);

    void RecvFromUe(Ptr<Socket> socket);

    void RecvFromOtherGnb(Ptr<Socket> socket);

    void UpdateUeState(uint64_t id, UeState state);

    void CompleteMigrationAtTarget(uint64_t ueId, uint64_t oldGnbId);

    virtual void StartApplication(); // Called at time specified by Start

    virtual void StopApplication(); // Called at time specified by Stop
};

} // namespace ns3

#endif
