#ifndef UE_APPLICATION_H
#define UE_APPLICATION_H

#include <ns3/application.h>
#include <ns3/socket.h>
#include <ns3/mc-ue-net-device.h>
#include <ns3/system-info.h>
#include <ns3/cf-e2e-calculator.h>
#include <ns3/cf-e2e-buffer.h>

namespace ns3
{
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

    virtual void SendInitRequest();
    
    virtual void SendTaskRequest();

    virtual void RecvTaskResult(Ptr<Packet> p);

    void SetOffloadAddress(Ipv4Address address, uint32_t port);

    void SwitchOffloadAddress(Ipv4Address newAddress, uint32_t newPort);

    void InitSocket();

    void HandleRead(Ptr<Socket> socket);

    void HandlePacket(Ptr<Packet> p);

    void RecvFromGnb(Ptr<Packet> p);

    void SendPacketToGnb(Ptr<Packet> p);

    void E2eTrace(CfRadioHeader cfRHd);

  protected:
    void DoDispose();

    Ipv4Address m_offloadAddress;

    uint16_t m_offloadPort;

    uint64_t m_ueId;

    uint64_t m_offloadGnbId;

    uint64_t m_taskId;

    Ptr<Socket> m_socket;

    uint16_t m_minSize;

    uint32_t m_requestDataSize;  // bytes

    uint32_t m_uploadPacketSize; // bytes

    double m_period; // ms

    Ptr<CfranSystemInfo> m_cfranSystemInfo;

    Ptr<mmwave::McUeNetDevice> m_mcUeNetDev;

  private:
    virtual void StartApplication(); // Called at time specified by Start

    virtual void StopApplication(); // Called at time specified by Stop
    uint32_t m_taskNow;

    Ptr<CfE2eBuffer> m_cfE2eBuffer;
    Ptr<CfE2eCalculator> m_cfE2eCalculator;

    TracedCallback<uint64_t, uint64_t, uint64_t> m_txRequestTrace;
    TracedCallback<uint64_t, uint64_t, uint64_t> m_rxResultTrace;
};

} // namespace ns3

#endif