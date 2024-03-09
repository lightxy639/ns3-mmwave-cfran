#ifndef UE_APPLICATION_H
#define UE_APPLICATION_H

#include <ns3/application.h>
#include <ns3/socket.h>

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

    virtual void SendTaskRequest();

    virtual void RecvTaskResult(Ptr<Packet> p);

    void SetOffloadAddress(Address address, uint32_t port);

    void SwitchOffloadAddress(Address newAddress, uint32_t newPort);

    void InitSocket();

    void HandleRead(Ptr<Socket> socket);


  protected:
    void DoDispose() override = 0;

    Address m_offloadAddress;

    uint16_t m_offloadPort;

    uint64_t m_ueId;

    uint64_t m_taskId;

    Ptr<Socket> m_socket;

    uint8_t m_minSize;

  private:
    virtual void StartApplication() = 0; // Called at time specified by Start

    virtual void StopApplication() = 0; // Called at time specified by Stop
};

} // namespace ns3

#endif