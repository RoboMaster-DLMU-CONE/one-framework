#ifndef OF_UNITS_HPP
#define OF_UNITS_HPP

#include <cstdint>
#include <mp-units/framework.h>
#include <mp-units/systems/angular.h>
#include <mp-units/systems/angular/units.h>
#include <mp-units/systems/si.h>
#include <mp-units/systems/si/unit_symbols.h>
#include <mp-units/systems/si/units.h>

namespace OF::Units
{
    using mp_units::quantity;
    using mp_units::quantity_point;
    using mp_units::angular::degree;
    using mp_units::angular::radian;
    using mp_units::angular::revolution;

    using mp_units::si::ampere;
    using mp_units::si::degree_Celsius;
    using mp_units::si::metre;
    using mp_units::si::milli;
    using mp_units::si::minute;
    using mp_units::si::newton;
    using mp_units::si::second;

    using Length = quantity<metre, float>;

    using Velocity = quantity<metre / second>;

    using Angle = quantity<radian, float>;
    using AngleDeg = quantity<degree, float>;

    using AngularVelocity = quantity<radian / second, float>;
    using AngulurVelocityDeg = quantity<degree / second, float>;

    using Round = quantity<revolution, int32_t>;
    using RPM = quantity<revolution / minute, float>;

    using Torque = quantity<newton * metre, float>;

    using Current = quantity<ampere, uint16_t>;
    using CurrentMilli = quantity<milli<ampere>, uint16_t>;
    using CurrentMilliF = quantity<milli<ampere>, float>;

    using Temperature =
    quantity_point<degree_Celsius, mp_units::si::absolute_zero, float>;

    namespace literals
    {
        using mp_units::angular::unit_symbols::deg;
        using mp_units::angular::unit_symbols::rad;
        using mp_units::angular::unit_symbols::rev;
        using mp_units::si::unit_symbols::deg_C;
        using mp_units::si::unit_symbols::m;
        using mp_units::si::unit_symbols::mm;
        using mp_units::si::unit_symbols::mA;
        using mp_units::si::unit_symbols::N;
        using mp_units::si::unit_symbols::s;
    } // namespace literals
}

#endif //OF_UNITS_HPP
