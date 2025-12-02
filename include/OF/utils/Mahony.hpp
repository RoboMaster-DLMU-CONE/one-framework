#ifndef OF_MAHONY_HPP
#define OF_MAHONY_HPP

#include <cmath>
#include <numbers>

namespace OF
{
    class Mahony
    {
    public:
        explicit Mahony(const float kp = 2.0f, const float ki = 0.001f) :
            kp(kp), ki(ki)
        {
            q[0] = 1.0f;
            q[1] = 0.0f;
            q[2] = 0.0f;
            q[3] = 0.0f;
        }

        // 更新函数，dt为两帧之间的时间间隔(秒)
        void update(float gx, float gy, float gz, float ax, float ay, float az, float dt)
        {
            float recipNorm;
            float halfvx, halfvy, halfvz;
            float halfex, halfey, halfez;
            float qa, qb, qc;

            // 1. 只有在加速度计数据有效时才进行融合
            if (!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f)))
            {

                // 2. 加速度计归一化
                recipNorm = invSqrt(ax * ax + ay * ay + az * az);
                ax *= recipNorm;
                ay *= recipNorm;
                az *= recipNorm;

                // 3. 估计重力方向（将四元数换算成旋转矩阵的第三行）
                // 注意：这里计算的是重力向量的一半，为了和后面的叉积计算抵消系数
                halfvx = q[1] * q[3] - q[0] * q[2];
                halfvy = q[0] * q[1] + q[2] * q[3];
                halfvz = q[0] * q[0] - 0.5f + q[3] * q[3];

                // 4. 计算误差（叉积）：测量重力 vs 估计重力
                halfex = (ay * halfvz - az * halfvy);
                halfey = (az * halfvx - ax * halfvz);
                halfez = (ax * halfvy - ay * halfvx);

                // 5. 积分误差（消除陀螺仪零偏）
                if (ki > 0.0f)
                {
                    integralFBx += ki * halfex * dt;
                    integralFBy += ki * halfey * dt;
                    integralFBz += ki * halfez * dt;
                    gx += integralFBx;
                    gy += integralFBy;
                    gz += integralFBz;
                }

                // 6. 比例增益修正
                gx += kp * halfex;
                gy += kp * halfey;
                gz += kp * halfez;
            }

            // 7. 四元数一阶龙格库塔积分
            gx *= (0.5f * dt);
            gy *= (0.5f * dt);
            gz *= (0.5f * dt);

            qa = q[0];
            qb = q[1];
            qc = q[2];

            q[0] += (-qb * gx - qc * gy - q[3] * gz);
            q[1] += (qa * gx + qc * gz - q[3] * gy);
            q[2] += (qa * gy - qb * gz + q[3] * gx);
            q[3] += (qa * gz + qb * gy - qc * gx);

            // 8. 四元数归一化
            recipNorm = invSqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
            q[0] *= recipNorm;
            q[1] *= recipNorm;
            q[2] *= recipNorm;
            q[3] *= recipNorm;
        };

        // 获取欧拉角 (单位：度)
        void getEulerAngle(float& pitch, float& roll, float& yaw)
        {
            // 这里的转换公式取决于你的定义的旋转顺序，通常RM使用 Z-Y-X
            // 结果为弧度，如果需要角度需 * 57.29578f

            // Roll (绕X轴)
            roll = atan2f(2.0f * (q[0] * q[1] + q[2] * q[3]), 1.0f - 2.0f * (q[1] * q[1] + q[2] * q[2]));

            // Pitch (绕Y轴)
            const float sinp = 2.0f * (q[0] * q[2] - q[3] * q[1]);
            if (fabsf(sinp) >= 1)
                pitch = copysignf(std::numbers::pi_v<float> / 2, sinp); // Use 90 degrees if out of range
            else
                pitch = asinf(sinp);

            // Yaw (绕Z轴)
            yaw = atan2f(2.0f * (q[0] * q[3] + q[1] * q[2]), 1.0f - 2.0f * (q[2] * q[2] + q[3] * q[3]));
        };

        // 获取四元数
        float q[4]{}; // q0(w), q1(x), q2(y), q3(z)

    private:
        float kp{};
        float ki{};
        float integralFBx{}, integralFBy{}, integralFBz{}; // 积分误差积累

        static float invSqrt(const float x)
        {
            return 1.0f / sqrtf(x);
        };
    };
}


#endif //OF_MAHONY_HPP
