#ifndef OF_JOINTINTEGRATOR_HPP
#define OF_JOINTINTEGRATOR_HPP

#include <algorithm>
#include <cmath>
#include <OF/utils/Units.hpp>

namespace OF::JointSetpointGenerator
{
    using namespace Units::literals;

    struct Config
    {
        Units::Angle min_angle; // 最小机械限位
        Units::Angle max_angle; // 最大机械限位
        Units::AngularVelocity max_speed; // 最大运行速度
        Units::Time dt = 1 * ms;
        float deadzone = 0.05f; // 遥控器死区
    };

    class Solver
    {
    public:
        Solver(const Config& config)
            :
            m_cfg(config), m_current_angle(0 * rad)
        {
        }

        void set_current(const Units::Angle current_angle)
        {
            m_current_angle = std::clamp(current_angle, m_cfg.min_angle, m_cfg.max_angle);
        }

        Units::Angle step(float input_ratio)
        {
            // 死区处理
            if (std::abs(input_ratio) < m_cfg.deadzone)
            {
                input_ratio = 0.0;
            }

            // 位置变化量 = 速度比例 * 最大速度 * 时间
            // mp-units 保证 [rad] = [1] * [rad/s] * [s]
            Units::Angle delta = input_ratio * m_cfg.max_speed * m_cfg.dt;

            // 累积
            m_current_angle += delta;

            // 限制目标值在安全范围内
            m_current_angle = std::clamp(m_current_angle, m_cfg.min_angle, m_cfg.max_angle);

            return m_current_angle;
        }

        [[nodiscard]] Units::Angle get_target() const { return m_current_angle; }

    private:
        Config m_cfg;
        Units::Angle m_current_angle; // 这是积分器维护的唯一状态
    };
}
#endif //OF_JOINTINTEGRATOR_HPP
