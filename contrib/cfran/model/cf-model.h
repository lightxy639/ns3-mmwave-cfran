#ifndef CF_MODEL_H
#define CF_MODEL_H

#include <ns3/application.h>
// #include <ns3/cf-application.h>
#include <ns3/ptr.h>

#include <string>

namespace ns3
{
// class CfApplication;

/**
 * \brief The model of computing force
 * \struct CfModel
 */
// class CfModel : public Object
struct CfModel
{
//   public:
    CfModel();

    CfModel(std::string cfType, float cfCapacity);

    // virtual ~CfModel();

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    // static TypeId GetTypeId();

    CfModel operator+(const CfModel&);
    CfModel operator-(const CfModel&);
    bool operator>=(const CfModel&);
    CfModel operator/(uint64_t num);

//   private:
    std::string m_cfType; // CPU, GPU, FPGA,...
    float m_cfCapacity;   // MIPS, TFLOPS,...
};

std::ostream &operator <<(std::ostream &os, const CfModel &cfModel);
std::istream &operator >>(std::istream &is, CfModel &cfModel);

ATTRIBUTE_HELPER_HEADER(CfModel);

/**
 * \brief The model of UE task
 * \struct UeTaskModel
 */
struct UeTaskModel
{
    UeTaskModel();
    UeTaskModel(uint64_t taskId, CfModel cfRequired, float cfLoad, float deadline);
    bool operator==(const UeTaskModel&);

    uint64_t m_taskId;
    CfModel m_cfRequired; // the computing force UE required (optional)
    float m_cfLoad;       // TFLOPs...
    float m_deadline;     // ms
    Ptr<Application> m_application;
};

} // namespace ns3
#endif
