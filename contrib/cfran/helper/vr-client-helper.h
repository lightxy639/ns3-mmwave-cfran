#ifndef VR_CLIENT_HELPER_H
#define VR_CLIENT_HELPER_H

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

class VrClientHelper : public Object
{
public:
    VrClientHelper();
    virtual ~VrClientHelper();
    /**
     *  Register this type.
     *  \return The object TypeId.
     */
    static TypeId GetTypeId(void);

    /**
     * Create one VR client application on each of the Nodes in the
     * NodeContainer
     * \param c The nodes on which to create the Applications.  The nodes
     *          are specified by a NodeContainer.
     */
    ApplicationContainer Install(NodeContainer c);
};
}

#endif