#ifndef CF_APPLICATION_H
#define CF_APPLICATION_H

#include <ns3/cf-model.h>
#include <ns3/cf-unit.h>
#include <ns3/application.h>

namespace ns3
{

struct CfModel;
struct UeTaskModel;
class CfUnit;

/**
 * \brief The implement of applications that require computing force
 */
class CfApplication : public Application
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    CfApplication();

    ~CfApplication() override;

    void SetCfUnit(Ptr<CfUnit> cfUnit);

    virtual void LoadTaskToCfUnit(uint64_t id, UeTaskModel ueTask) = 0;

    virtual void RecvTaskResult(uint64_t id, UeTaskModel ueTask) = 0;

  protected:
    void DoDispose() override = 0;

    Ptr<CfUnit> m_cfUnit;

  private:
    virtual void StartApplication() = 0; // Called at time specified by Start

    virtual void StopApplication() = 0; // Called at time specified by Stop

};

} // namespace ns3

#endif
