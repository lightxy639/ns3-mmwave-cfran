#ifndef CF_APPLICATION_HELPER_H
#define CF_APPLICATION_HELPER_H

#include <ns3/application-container.h>
#include <ns3/cf-application.h>
#include <ns3/node-container.h>
#include <ns3/object.h>

namespace ns3
{
/**
 * \ingroup cfran
 *
 * Create ue cf applications which offload tasks to gNBs or other servers
 */

class CfApplicationHelper : public Object
{
  public:
    CfApplicationHelper();
    virtual ~CfApplicationHelper();
    /**
     *  Register this type.
     *  \return The object TypeId.
     */
    static TypeId GetTypeId(void);

    void SetRemoteIdOffset(uint16_t remoteIdOffset);

    /**
     * Create one VR client application on each of the Nodes in the
     * NodeContainer
     * \param c The nodes on which to create the Applications.  The nodes
     *          are specified by a NodeContainer.
     */
    ApplicationContainer Install(NodeContainer c, bool isGnb = true);

  private:
    uint16_t m_remoteIdOffset; // The starting ID of the remote server, which depends on the number of base stations
    bool m_e2ModeCfApp;
    std::string m_e2ip;
    uint16_t m_e2port;
    uint16_t m_e2localPort;
};
} // namespace ns3

#endif