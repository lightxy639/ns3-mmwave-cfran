#ifndef CF_UNIT_UE_ISO
#define CF_UNIT_UE_ISO

#include <ns3/cf-application.h>
#include <ns3/cf-model.h>
#include <ns3/cf-unit.h>

#include <queue>

namespace ns3
{

class CfUnitUeIso : public CfUnit
{
  public:
    CfUnitUeIso();

    CfUnitUeIso(CfModel cf)
        : CfUnit(cf){};

    virtual ~CfUnitUeIso();
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    void DoDispose();

    void LoadUeTask(uint64_t ueId, UeTaskModel ueTask) override;

    void AddNewUe(uint64_t ueId) override;

    void DeleteUe(uint64_t ueId) override;

    void ExecuteUeTask(uint64_t ueId, UeTaskModel ueTask) override;

    void EndUeTask(uint64_t ueId, UeTaskModel ueTask) override;

    void ReAllocateCf();

  private:
    std::map<uint64_t, std::queue<UeTaskModel>> m_ueTask;

    std::map<uint64_t, CfModel> m_cfAllocation;
};

} // namespace ns3

#endif