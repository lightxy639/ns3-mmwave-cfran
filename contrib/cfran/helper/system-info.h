#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

#include <ns3/cf-model.h>
#include <ns3/address.h>
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
    struct UeInfo
    {
        uint64_t m_imsi; // imsi
        float m_taskPeriodity; // ms
        Ptr<mmwave::McUeNetDevice> m_mcUeNetDevice;
        UeTaskModel m_taskModel;
    };

    struct CellInfo
    {
        uint64_t m_id;
        Ptr<mmwave::MmWaveEnbNetDevice> m_mmwaveEnbNetDevice;
    };

  public:
    CfranSystemInfo();
    virtual ~CfranSystemInfo();

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    UeInfo GetUeInfo(uint64_t imsi);
    CellInfo GetCellInfo(uint64_t cellId);

  protected:
    // inherited from Object
    virtual void DoInitialize(void);

  private:
    std::map<uint16_t, UeInfo> m_ueInfo;
    std::map<uint16_t, CellInfo> m_cellInfo;
};
} // namespace ns3

#endif