#ifndef CF_UNIT_H
#define CF_UNIT_H

#include <ns3/cf-application.h>
#include <ns3/cf-model.h>
#include <ns3/net-device.h>

/**
 * \defgroup cfran Models
 *
 */

namespace ns3
{
class Node;
class CfApplication;

class CfUnit : public Object
{
  public:
    CfUnit();

    CfUnit(CfModel cf);

    virtual ~CfUnit();

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    virtual void DoDispose();

    void SetCfUnitId(uint64_t id);
    // inherited from NetDevice
    virtual void SetNode(Ptr<Node> node);

    virtual void SetCfApplication(Ptr<CfApplication> cfApp);

    /**
     * \brief Load upcoming tasks
     * \param id the id of user
     * \param ueTask the model of this task
     */

    CfModel GetCf();

    virtual void LoadUeTask(uint64_t ueId, UeTaskModel ueTask) = 0;

    virtual void AddNewUe(uint64_t ueId) = 0;

    virtual void DeleteUe(uint64_t ueId) = 0;

    virtual void ExecuteUeTask(uint64_t ueId, UeTaskModel ueTask) = 0;

    virtual void EndUeTask(uint64_t ueId, UeTaskModel ueTask) = 0;

  protected:
    Ptr<Node> m_node; ///< the node
    uint64_t m_id;    ///< the id of computing force unit
    CfModel m_cf;     ///< the total computing force of CfUnit
    Ptr<CfApplication> m_cfApplication;
};
} // namespace ns3
#endif