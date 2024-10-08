#ifndef UE_APPLICATION_H
#define UE_APPLICATION_H

#include "cf-radio-header.h"

#include <ns3/application.h>
#include <ns3/cf-e2e-calculator.h>
#include <ns3/mc-ue-net-device.h>
#include <ns3/socket.h>
#include <ns3/system-info.h>
// #include <ns3/cf-e2e-buffer.h>
#include <ns3/cf-time-buffer.h>
#include <ns3/multi-packet-manager.h>

namespace ns3
{
class CfranSystemInfo;
/**
 * \brief The implementation of applications at UE which offload tasks to gNB.
 */
class UeCfApplication : public Application
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    UeCfApplication();

    ~UeCfApplication() override;

    void SetUeId(uint64_t ueId);

    void SetMcUeNetDevice(Ptr<mmwave::McUeNetDevice> mcUeNetDev);

    Ptr<mmwave::McUeNetDevice> GetMcUeNetDevice();

    virtual void SendInitRequest();

    virtual void SendTaskRequest(uint64_t offloadPointId);

    void SendTerminateCommand();

    virtual void RecvTaskResult(Ptr<Packet> p);

    // void SetOffloadAddress(Ipv4Address address, uint32_t port);

    // void SwitchOffloadAddress(Ipv4Address newAddress, uint32_t newPort);

    // void InitSocket();

    void InitGnbSocket();

    void InitRemoteSocket();

    void HandleRead(Ptr<Socket> socket);

    void HandlePacket(Ptr<Packet> p);

    void RecvFromNetwork(Ptr<Packet> p);

    void SendPacketToGnb(Ptr<Packet> p);

    void E2eTrace(CfRadioHeader cfRHd);

    void ChangeStatus();

  protected:
    void DoDispose();

    uint64_t m_ueId;

    uint64_t m_accessGnbId;

    uint64_t m_offloadPointId;

    // Ipv4Address m_offloadAddress;

    // uint16_t m_offloadPort;
    bool m_enableIdealProtocol;
    
    bool m_active;

    bool m_randomArrivalDeparture;

    uint16_t m_ueGnbPort;

    uint16_t m_ueRemotePort;

    uint64_t m_taskId;

    Ptr<Socket> m_socket;

    Ptr<Socket> m_gnbSocket;

    Ptr<Socket> m_remoteSocket;

    uint16_t m_minSize;

    // uint32_t m_requestDataSize;  // bytes

    uint32_t m_uploadPacketSize; // bytes

    double m_period; // ms

    double m_randomActionPeriod; // ms

    Ptr<CfranSystemInfo> m_cfranSystemInfo;

    Ptr<mmwave::McUeNetDevice> m_mcUeNetDev;

  private:
    virtual void StartApplication(); // Called at time specified by Start

    virtual void StopApplication(); // Called at time specified by Stop

    uint32_t m_taskNow;

    EventId m_reqEventId;

    // Ptr<CfE2eBuffer> m_cfE2eBuffer;
    Ptr<CfE2eCalculator> m_cfE2eCalculator;
    Ptr<CfTimeBuffer> m_cfTimeBuffer;

    Ptr<MultiPacketManager> m_downlinkResultManager;

    TracedCallback<uint64_t, uint64_t, uint64_t, TimeType, OffloadPosition> m_sendRequestTrace;
    TracedCallback<uint64_t, uint64_t, uint64_t, TimeType, OffloadPosition> m_recvResultTrace;
};

} // namespace ns3

#endif