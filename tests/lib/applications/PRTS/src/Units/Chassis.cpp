#include "Chassis.hpp"

#include <OF/lib/applications/PRTS/PrtsRegistrar.hpp>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(Chassis, LOG_LEVEL_INF);

REGISTER_UNIT(Chassis)

struct ChassisSetPIDOpts
{
    static constexpr std::array<Prts::OptionDesc, 4> value{{
        {"motor", Prts::OptionType::INT},
        {"kp", Prts::OptionType::DOUBLE},
        {"ki", Prts::OptionType::DOUBLE},
        {"kd", Prts::OptionType::DOUBLE}
    }};
};

int Chassis::setPID(const int motor, const double kp, const double ki, const double kd)
{
    array[motor] = {
        kp, ki, kd
    };
    return 0;
}

PRTS_COMMAND_T(Chassis, setPID, "设置PID参数", ChassisSetPIDOpts);
PRTS_ELEMENT_T(Chassis, motor1_kp, "text", 0, 100, getMotor1Kp)
PRTS_ELEMENT_T(Chassis, motor2_kp, "text", 0, 100, getMotor2Kp)
PRTS_ELEMENT_T(Chassis, motor3_kp, "text", 0, 100, getMotor3Kp)
PRTS_ELEMENT_T(Chassis, motor4_kp, "text", 0, 100, getMotor4Kp)
