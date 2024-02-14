#ifndef CFRAN_HELPER_H
#define CFRAN_HELPER_H

#include <ns3/cf-unit.h>
#include <ns3/node-container.h>
#include <ns3/object-factory.h>

namespace ns3
{

// Each class should be documented using Doxygen,
// and have an \ingroup cfran directive

/* ... */
/**
 * \ingroup cfran
 *
 * Creation and configuration of cfunit
 */
class CfRanHelper : public Object
{
  public:
    CfRanHelper();
    virtual ~CfRanHelper();

    /**
     *  Register this type.
     *  \return The object TypeId.
     */
    static TypeId GetTypeId();
    // virtual void DoDispose(void);

    /**
     * \brief Aggregate cfUnits to nodes
     * \param c the NodeContainer consisting of nodes where the CfUnit is about to be installed
     * \param cfUnitObj the ObjectFactory of the CfUnit to associate to nodes
     */
    void InstallCfUnit(NodeContainer c, ObjectFactory cfUnitObj);

    // void InstallCfApplication(NodeContainer)
};
} // namespace ns3

#endif /* CFRAN_HELPER_H */
