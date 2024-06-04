#ifndef CF_APPLICATION_H
#define CF_APPLICATION_H

#include <ns3/application.h>
#include <ns3/cf-e2e-calculator.h>
#include <ns3/cf-unit.h>
#include <ns3/multi-packet-manager.h>
#include <ns3/oran-interface.h>
#include <ns3/socket.h>
#include <ns3/system-info.h>

namespace ns3
{
class CfUnit;

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
    virtual ~CfApplication();

    virtual void DoDispose();

    void SetCfUnit(Ptr<CfUnit> cfUnit);

    virtual void SetE2Termination(Ptr<E2Termination> e2term);

    Ptr<E2Termination> GetE2Termination() const;

    void UpdateUeState(uint64_t id, UeState state);

    virtual void RecvFromUe(Ptr<Socket> socket) = 0;
    virtual void SendPacketToUe(uint64_t ueId, Ptr<Packet> packet) = 0;

    virtual void RecvTaskResult(uint64_t id, UeTaskModel ueTask) = 0;
    // virtual void LoadTaskToCfUnit(uint64_t id, UeTaskModel ueTask);

  protected:
    Ptr<CfUnit> m_cfUnit;
    Ptr<CfranSystemInfo> m_cfranSystemInfo;
    Ptr<Socket> m_socket;
    Ptr<MultiPacketManager> m_multiPacketManager;
    Ptr<E2Termination> m_e2term;

    std::map<uint64_t, UeState> m_ueState;

    uint16_t m_port;
    uint32_t m_initDelay;
    uint32_t m_defaultPacketSize;

    Ptr<CfE2eCalculator> m_cfE2eCalaulator;
    TracedCallback<uint64_t, uint64_t, uint64_t> m_recvRequestTrace;
    TracedCallback<uint64_t, uint64_t, uint64_t> m_addTaskTrace;
    TracedCallback<uint64_t, uint64_t, uint64_t> m_getResultTrace;
    TracedCallback<uint64_t, uint64_t, uint64_t> m_sendResultTrace;
};
} // namespace ns3

#endif