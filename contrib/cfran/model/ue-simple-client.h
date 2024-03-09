#ifndef UE_VR_CLIENT_H
#define UE_VR_CLIENT_H

#include <ns3/ue-cf-application.h>

namespace ns3
{
/**
 * \ingroup cfran
 * The VR client implementation with uplink
 */

class UeVrClient : public UeCfApplication
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    UeVrClient();

    ~UeVrClient();

  protected:
    virtual void DoDispose();

  private:
    virtual void StartApplication(void);

    virtual void StopApplication(void);
}
} // namespace ns3
#endif