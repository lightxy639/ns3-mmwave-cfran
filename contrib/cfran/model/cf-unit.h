#ifndef CF_UNIT_NET_DEVICE_H
#define CF_UNIT_NET_DEVICE_H

#include <ns3/cf-model.h>
#include <ns3/net-device.h>

namespace ns3
{

class Node;

/**
 * \defgroup cfran Models
 *
 */

/**
 * \ingroup cfran
 *
 * The computing force unit implementation
 */
class CfUnit : public Object
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    CfUnit();
    CfUnit(CfModel cf);
    virtual ~CfUnit();

    static TypeId GetTypeId();
    virtual void DoDispose();

    // inherited from NetDevice
    virtual void SetNode(Ptr<Node> node);

    /**
     * \brief Allocate computing force for tasks (periodic/event-trigger-based)
     */
    void ScheduleTasks();

    /**
     * \brief Add new task to schedule map
     * \param id the id of user
     * \param ueTask the model of this task
     */
    void AddNewUeTaskForSchedule(uint16_t id, UeTaskModel ueTask);

    /**
     * \brief Delete the task that have already been scheduled
     * \param id the id of user
     * \param ueTask the model of this task
     */
    void DeleteUeTask(uint16_t id, UeTaskModel ueTask);

    /**
     * \brief Allocate computing force in m_cfAllocation
     * \param id the id of user
     * \param ueTask the model of this task
     */
    void AllocateCf(uint16_t id, UeTaskModel ueTask, CfModel cfModel);

    /**
     * \brief Freeup the computing force
     * \param id the id of user
     * \param ueTask the model of this task
     */
    void FreeCf(uint16_t id, UeTaskModel ueTask);
    /**
     * \brief Allocate computing force for one task according to command
     * \param id the id of user
     * \param ueTask the model of this task
     */
    void ScheduleNewTaskWithCommand(uint16_t id, UeTaskModel ueTask);

    /**
     * \brief Execute a specific task
     * \param id the id of user
     * \param ueTask the model of this task
     */
    void ExecuteTask(uint16_t id, UeTaskModel ueTask);

    /**
     * \brief End a specific task
     * \param id the id of user
     * \param ueTask the model of this task
     */
    void EndTask(uint16_t id, UeTaskModel ueTask);

    /**
     * \brief Output the computing force allocation map
     */
    void OutputCfAllocationInfo();

    CfModel GetCf();

  protected:
    // inherited from Object
    virtual void DoInitialize(void);

  private:
    Ptr<Node> m_node; ///< the node
    uint16_t m_id;    ///< the id of computing force unit
    CfModel m_cf;
    CfModel m_freeCf;
    std::map<uint16_t, std::vector<UeTaskModel>> m_ueTask; ///< the tasks of UEs, id -> tasks
    ///< if of UE -> id of task -> computing force allocated
    std::map<uint16_t, std::map<uint16_t, CfModel>> m_cfAllocation;

    bool m_enableAutoSchedule; ///< schedule computing force autonomously of receieve command
};
} // namespace ns3

#endif /* CF_UNIT_NET_DEVICE_H */
