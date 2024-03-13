#ifndef CF_APPLICATION_H
#define CF_APPLICATION_H

#include <ns3/application.h>
#include <ns3/cf-model.h>
#include <ns3/cf-unit.h>
#include <ns3/epc-x2.h>
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

    // virtual 

    virtual void LoadTaskToCfUnit(uint64_t id, UeTaskModel ueTask);

    virtual void RecvTaskResult(uint64_t id, UeTaskModel ueTask);

    // If the application is installed on a gNB, the results will be sent directly through
    // the gNB
    virtual void SendTaskResultToUserFromGnb(uint64_t id, Ptr<Packet> packet);

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

    std::map<uint16_t, Ptr<CfX2IfaceInfo>> m_cfX2InterfaceSockets;
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
    void HandleRead(Ptr<Socket> socket);

    virtual void StartApplication(); // Called at time specified by Start

    virtual void StopApplication(); // Called at time specified by Stop
};

} // namespace ns3

#endif
