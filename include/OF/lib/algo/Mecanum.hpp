#ifndef OF_MECANUM_HPP
#define OF_MECANUM_HPP
#include <OF/utils/Units.hpp>

namespace OF::Algo::Mecanum
{
    using namespace Units;
    using namespace Units::literals;

    struct Config
    {
        Length wheel_radius; ///< 轮子半径
        Length track_width; ///< 轮距 (左右轮中心距离)
        Length wheel_base; ///< 轴距 (前后轮中心距离)

        [[nodiscard]] constexpr Length geometry_factor() const
        {
            return (track_width + wheel_base) / 2.0f;
        }
    };

    struct ChassisSpeeds
    {
        Velocity vx;
        Velocity vy;
        AngularVelocity vw;
    };

    struct WheelStates
    {
        AngularVelocity fl; // Front-Left
        AngularVelocity fr; // Front-Right
        AngularVelocity bl; // Back-Left
        AngularVelocity br; // Back-Right
    };

    class Solver
    {
    public:
        constexpr explicit Solver(const Config& config) : m_config(config)
        {
        }

        [[nodiscard]] constexpr WheelStates inverse(const ChassisSpeeds& cmd) const
        {
            const auto K = m_config.geometry_factor();
            // K * vw = [m] * [rad/s] = [m·rad/s]
            // 需要除以 rad
            auto v_fl = cmd.vx - cmd.vy - K * cmd.vw / rad;
            auto v_fr = cmd.vx + cmd.vy + K * cmd.vw / rad;
            auto v_bl = cmd.vx + cmd.vy - K * cmd.vw / rad;
            auto v_br = cmd.vx - cmd.vy + K * cmd.vw / rad;
            return {
                v_fl / m_config.wheel_radius * rad,
                v_fr / m_config.wheel_radius * rad,
                v_bl / m_config.wheel_radius * rad,
                v_br / m_config.wheel_radius * rad
            };
        }

        [[nodiscard]] constexpr ChassisSpeeds forward(const WheelStates& wheels) const
        {
            const auto K = m_config.geometry_factor();
            auto v_fl = wheels.fl * m_config.wheel_radius / rad;
            auto v_fr = wheels.fr * m_config.wheel_radius / rad;
            auto v_bl = wheels.bl * m_config.wheel_radius / rad;
            auto v_br = wheels.br * m_config.wheel_radius / rad;
            return {
                .vx = (v_fl + v_fr + v_bl + v_br) / 4.0,
                .vy = (-v_fl + v_fr + v_bl - v_br) / 4.0,
                // 分子是 [m/s]，分母 K 是 [m]
                // [m/s] / [m] = [1/s] (Frequency)。我们需要 [rad/s]，所以必须 * rad
                .vw = ((-v_fl + v_fr - v_bl + v_br) / (4.0 * K)) * rad
            };
        }

    private:
        Config m_config;
    };
}

#endif //OF_MECANUM_HPP
