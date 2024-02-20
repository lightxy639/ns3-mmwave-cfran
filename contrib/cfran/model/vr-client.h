#ifndef VR_CLIENT_H
#define VR_CLIENT_H

#include <ns3/cf-application.h>

namespace ns3
{
/**
 * \ingroup cfran
 * The VR client implementation
 */

class VrClient : public Application
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    VrClient();

    ~VrClient();

    void HandlePacket(Ptr<Packet> p);

  protected:
    virtual void DoDispose(void);

  private:
    virtual void StartApplication(void);

    virtual void StopApplication(void);

};
} // namespace ns3

#endif