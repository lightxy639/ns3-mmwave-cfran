#ifndef VR_SERVER_HELPER_H
#define VR_SERVER_HELPER_H

#include <ns3/node-container.h>
#include <ns3/object.h>
#include <ns3/application-container.h>

namespace ns3
{

/**
 * \ingroup cfran
 * 
 * Create VR server applications which use the computional resources
 *  of CfUnit
 */
class VrServerHelper : public Object
{
  public:
    VrServerHelper();
    virtual ~VrServerHelper();

    /**
     *  Register this type.
     *  \return The object TypeId.
     */
    static TypeId GetTypeId(void);

    /**
     * Create one udp echo server application on each of the Nodes in the
     * NodeContainer
     * \param c The nodes on which to create the Applications.  The nodes
     *          are specified by a NodeContainer.
     */
    ApplicationContainer Install(NodeContainer c);
};
} // namespace ns3

#endif