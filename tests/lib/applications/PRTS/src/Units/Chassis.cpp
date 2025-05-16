#include "Chassis.hpp"

#include <OF/lib/applications/PRTS/PrtsRegistrar.hpp>

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
