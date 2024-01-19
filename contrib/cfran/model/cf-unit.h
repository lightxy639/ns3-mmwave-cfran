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
    CfUnit();
    CfUnit(CfModel cf);
    virtual ~CfUnit();

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    virtual void DoDispose();

    // inherited from NetDevice
    virtual void SetNode(Ptr<Node> node);

    /**
     * \brief Allocate computing force for tasks and execute them (periodic/event-trigger-based)
     */
    void ScheduleTasks(); // TODO

    /**
     * \brief Add new task to UE task map
     * \param id the id of user
     * \param ueTask the model of this task
     */
    void AddNewUeTaskForSchedule(uint16_t id, UeTaskModel ueTask);

    /**
     * \brief Allocate computing force for one task according to command and execute it
     * \param id the id of user
     * \param ueTask the model of this task
     */
    void ScheduleNewTaskWithCommand(uint16_t id, UeTaskModel ueTask);

    /**
     * \brief Allocate computing force by modifying  allcation map
     * \param id the id of user
     * \param ueTask the model of this task
     */
    void AllocateCf(uint16_t id, UeTaskModel ueTask, CfModel cfModel);

    /**
     * \brief Execute a specific task for the theoretical time
     * \param id the id of user
     * \param ueTask the model of this task
     */
    void ExecuteTask(uint16_t id, UeTaskModel ueTask);

    /**
     * \brief End a specific task after the theoretical time
     * \param id the id of user
     * \param ueTask the model of this task
     */
    void EndTask(uint16_t id, UeTaskModel ueTask);

    /**
     * \brief Freeup the computing force by modifying allocation map
     * \param id the id of user
     * \param ueTask the model of this task
     */
    void FreeCf(uint16_t id, UeTaskModel ueTask);

    /**
     * \brief Delete the info of the task by modifying UE task map
     * \param id the id of user
     * \param ueTask the model of this task
     */
    void DeleteUeTask(uint16_t id, UeTaskModel ueTask);

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
    CfModel m_cf;     ///< the total computing force of cfunit
    CfModel m_freeCf; ///< the free computing force
    // std::map<uint16_t, std::vector<UeTaskModel>> m_ueTask; ///< the tasks of UEs, id -> tasks
    ///< id of UE -> id of task -> ueTaskModel
    std::map<uint16_t, std::map<uint16_t, UeTaskModel>> m_ueTask;
    ///< id of UE -> id of task -> computing force allocated
    std::map<uint16_t, std::map<uint16_t, CfModel>> m_cfAllocation;

    bool m_enableAutoSchedule; ///< schedule computing force autonomously of receieve command
};
} // namespace ns3

#endif /* CF_UNIT_NET_DEVICE_H */
