#ifndef CF_MODEL_H
#define CF_MODEL_H

#include <string>

namespace ns3
{
/**
 * \brief The model of computing force
 * \struct CfModel
 */
struct CfModel
{
    CfModel();
    CfModel(std::string cfType, float cfCapacity);
    CfModel operator+(const CfModel&);
    CfModel operator-(const CfModel&);
    bool operator>=(const CfModel&);
    CfModel operator/ (uint16_t num);

    std::string m_cfType; // CPU, GPU, FPGA,...
    float m_cfCapacity;   // MIPS, TFLOPS,...
};

/**
 * \brief The model of UE task
 * \struct UeTaskModel
 */
struct UeTaskModel
{
    UeTaskModel();
    UeTaskModel(uint16_t taskId, CfModel cfRequired, float cfLoad, float deadline);
    bool operator==(const UeTaskModel&);

    uint16_t m_taskId;
    CfModel m_cfRequired; // the computing force UE required (optional)
    float m_cfLoad;       // TFLOPs...
    float m_deadline;     // ms
};

} // namespace ns3
#endif