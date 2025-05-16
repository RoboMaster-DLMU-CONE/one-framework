#ifndef CHASSIS_HPP
#define CHASSIS_HPP

#include <OF/lib/applications/PRTS/PrtsManager.hpp>
#include <OF/lib/applications/Unit/UnitRegistry.hpp>

using namespace OF;

class Chassis final : public Unit
{
public:
    DEFINE_UNIT_DESCRIPTOR(Chassis, "Chassis", "Chassis test unit", 1024, 5)

    virtual void run() override
    {
        k_sleep(K_SECONDS(1));
    };

    int setPID(int motor, double kp, double ki, double kd);

    std::array<std::array<double, 3>, 4> array{};
};

#endif //CHASSIS_HPP
