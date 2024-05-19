#ifndef REMOTE_CF_APPLICATION_H
#define REMOTE_CF_APPLICATION_H

#include <ns3/cf-application.h>

namespace ns3
{
class RemoteCfApplication : public CfApplication
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    RemoteCfApplication();

    ~RemoteCfApplication() override;

    void SetServerId(uint64_t serverId);

    uint64_t GetServerId();

    void SendPacketToUe(uint64_t ueId, Ptr<Packet> packet) override;

    void RecvFromUe(Ptr<Socket> socket) override;

    void RecvTaskResult(uint64_t id, UeTaskModel ueTask) override;


  private:
    virtual void StartApplication();

    virtual void StopApplication();

    uint64_t m_serverId;
};
} // namespace ns3

#endif
