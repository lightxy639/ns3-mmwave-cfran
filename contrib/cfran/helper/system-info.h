#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

#include <ns3/address.h>
#include <ns3/cf-model.h>
#include <ns3/mc-ue-net-device.h>
#include <ns3/mmwave-enb-net-device.h>

#include <map>

namespace ns3
{
/**
 * \ingroup cfran
 *
 * Store some global information and omit the information synchronization process
 */
class CfranSystemInfo : public Object
{
  public:
    enum OffloadPointType
    {
        Gnb = 0,
        Remote = 1
    };

    struct UeInfo
    {
        uint64_t m_imsi; // imsi
        Ipv4Address m_ipAddr;
        uint16_t m_port;
        float m_taskPeriodity; // ms
        Ptr<mmwave::McUeNetDevice> m_mcUeNetDevice;
        UeTaskModel m_taskModel;
    };

    struct CellInfo
    {
        uint64_t m_id;
        Ptr<mmwave::MmWaveEnbNetDevice> m_mmwaveEnbNetDevice;
        Ipv4Address m_ipAddrToUe;
        uint16_t m_portToUe;
    };

    struct RemoteInfo
    {
        uint64_t m_id;
        Ipv4Address m_ipAddr;
        uint16_t m_port;
        float m_hostGwLatency; //ms
    };

    struct WiredLatencyInfo
    {
        float m_s1ULatency; // ms
        float m_x2Latency;  // ms
    };

    CfranSystemInfo();
    virtual ~CfranSystemInfo();

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    UeInfo GetUeInfo(uint64_t imsi);
    CellInfo GetCellInfo(uint64_t cellId);
    RemoteInfo GetRemoteInfo(uint64_t remoteId);
    WiredLatencyInfo GetWiredLatencyInfo();
    OffloadPointType GetOffladPointType(uint64_t offloadId);

    void AddUeInfo(uint64_t imsi, UeInfo ueInfo);
    void AddCellInfo(uint64_t cellId, CellInfo cellInfo);
    void AddRemoteInfo(uint64_t remoteId, RemoteInfo remoteInfo);
    void SetWiredLatencyInfo(WiredLatencyInfo wiredInfo);

  protected:
    // inherited from Object
    virtual void DoInitialize(void);

  private:
    std::map<uint64_t, UeInfo> m_ueInfo;
    std::map<uint64_t, CellInfo> m_cellInfo;
    std::map<uint64_t, RemoteInfo> m_remoteInfo;
    WiredLatencyInfo m_wiredLatencyInfo;
};
} // namespace ns3

#endif
