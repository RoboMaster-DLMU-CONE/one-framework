#include <OF/lib/ImuHub/ImuHub.hpp>
#include <OF/utils/Mahony.hpp>
#include <OF/utils/CCM.h>

#include <cerrno>
#include <cmath>

#include <zephyr/logging/log.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(ImuHub, CONFIG_IMU_HUB_LOG_LEVEL);

OF_CCM_ATTR uint8_t g_cnt{};
constexpr uint16_t UPDATE_COUNT = CONFIG_IMU_HUB_PUBLISH_EVERY_N_FRAME / 2 * 2;
BUILD_ASSERT(CONFIG_IMU_HUB_PUBLISH_EVERY_N_FRAME != 0, "CONFIG_IMU_HUB_DATA_UPDATE_FRAME_CNT should not be 0!");

namespace OF
{
    OF_CCM_ATTR Mahony g_mahony{1.5f, 0.0f};
    OF_CCM_ATTR IMUData g_imu_data{};
    OF_CCM_ATTR uint64_t g_prev_timestamp{};
    RTIO_DEFINE_WITH_MEMPOOL(imu_rtio_ctx, 16, 16, 16, 512, sizeof(void *));

#ifdef CONFIG_IMU_HUB_INITIAL_CALIBRATION
    // 使用 double 防止累加过程中精度丢失
    OF_CCM_ATTR struct
    {
        double x;
        double y;
        double z;
    } g_gyro_accum{};

    OF_CCM_ATTR IMUData::Vector3 g_gyro_bias{}; // 存储计算出的零偏
    OF_CCM_ATTR uint32_t g_calib_sample_count{};
    OF_CCM_ATTR int64_t g_calib_start_time = -1;
    OF_CCM_ATTR bool g_is_calibrated = false;
#endif
    namespace
    {
        struct AsyncSensorContext
        {
            const device* dev = nullptr;
            const rtio_iodev* iodev = nullptr;
            sensor_chan_spec* channels = nullptr;
            size_t channel_count = 0;
            sensor_channel channel_type = SENSOR_CHAN_ALL;
            bool enabled = false;
        };

        OF_CCM_ATTR sensor_chan_spec accel_channels[] = {{SENSOR_CHAN_ACCEL_XYZ, 0},};
        OF_CCM_ATTR sensor_chan_spec gyro_channels[] = {{SENSOR_CHAN_GYRO_XYZ, 0},};

        OF_CCM_ATTR sensor_read_config accel_read_cfg = {.sensor = nullptr, .is_streaming = false,
                                                         .channels = accel_channels,
                                                         .count = std::size(accel_channels),
                                                         .max = std::size(accel_channels)};
        OF_CCM_ATTR sensor_read_config gyro_read_cfg = {.sensor = nullptr, .is_streaming = false,
                                                        .channels = gyro_channels,
                                                        .count = std::size(gyro_channels),
                                                        .max = std::size(gyro_channels)};

        RTIO_IODEV_DEFINE(imu_accel_iodev, &__sensor_iodev_api, &accel_read_cfg);
        RTIO_IODEV_DEFINE(imu_gyro_iodev, &__sensor_iodev_api, &gyro_read_cfg);

        OF_CCM_ATTR AsyncSensorContext accel_ctx = {.iodev = &imu_accel_iodev, .channels = accel_channels,
                                                    .channel_count = std::size(accel_channels),
                                                    .channel_type = SENSOR_CHAN_ACCEL_XYZ};
        OF_CCM_ATTR AsyncSensorContext gyro_ctx = {.iodev = &imu_gyro_iodev, .channels = gyro_channels,
                                                   .channel_count = std::size(gyro_channels),
                                                   .channel_type = SENSOR_CHAN_GYRO_XYZ};

        constexpr AsyncSensorContext* const contexts[] = {&accel_ctx, &gyro_ctx};

        K_THREAD_STACK_DEFINE(imu_rtio_stack, CONFIG_IMU_HUB_RTIO_STACK_SIZE);
        OF_CCM_ATTR k_thread imu_rtio_thread_data;
        OF_CCM_ATTR bool imu_rtio_thread_started;

        constexpr int kRtioThreadPriority = (CONFIG_IMU_HUB_RTIO_THREAD_PRIORITY == 0)
            ? K_LOWEST_APPLICATION_THREAD_PRIO
            : CONFIG_IMU_HUB_RTIO_THREAD_PRIORITY;
        static_assert(
            kRtioThreadPriority >= K_HIGHEST_APPLICATION_THREAD_PRIO && kRtioThreadPriority <=
            K_LOWEST_APPLICATION_THREAD_PRIO, "Invalid IMU Hub RTIO thread priority");

        float q31_to_float(const q31_t value, const int8_t shift)
        {
            return ldexpf(static_cast<float>(value), shift - 31);
        }

        int configure_context(AsyncSensorContext& ctx, const device* dev)
        {
            if (dev == nullptr)
                return -ENODEV;
            ctx.dev = dev;
            ctx.enabled = true;
            int err = sensor_reconfigure_read_iodev(ctx.iodev, dev, ctx.channels, ctx.channel_count);
            if (err != 0)
            {
                LOG_ERR("Failed to bind %s: %d", dev->name, err);
                ctx.enabled = false;
                return err;
            }
            constexpr sensor_value freq = {.val1 = CONFIG_IMU_HUB_SAMPLING_FREQUENCY, .val2 = 0};
            sensor_attr_set(dev, SENSOR_CHAN_ALL, SENSOR_ATTR_SAMPLING_FREQUENCY, &freq);
            return 0;
        }

        int submit_async_read(AsyncSensorContext& ctx)
        {
            if (!ctx.enabled)
                return -EACCES;
            int err = sensor_read_async_mempool(ctx.iodev, &imu_rtio_ctx, &ctx);
            if (err != 0)
                LOG_ERR("sensor_read_async_mempool failed: %d", err);
            return err;
        }

        void start_worker_thread()
        {
            if (imu_rtio_thread_started)
                return;
            k_thread_create(&imu_rtio_thread_data, imu_rtio_stack, K_THREAD_STACK_SIZEOF(imu_rtio_stack),
                            ImuHub::async_worker_thread, nullptr, nullptr, nullptr,
                            kRtioThreadPriority, 0, K_NO_WAIT);
            k_thread_name_set(&imu_rtio_thread_data, "imu_rtio");
            imu_rtio_thread_started = true;
        }

        // 辅助函数：根据加速度计计算初始Roll/Pitch，避免Mahony启动时收敛慢
        void align_mahony_to_gravity(float ax, float ay, float az)
        {
            // 简单的归一化
            const float norm = std::sqrt(ax * ax + ay * ay + az * az);
            if (norm < 1e-4f)
                return;
            ax /= norm;
            ay /= norm;
            az /= norm;

            // 计算初始欧拉角 (假设Yaw=0)
            // Roll: 绕X轴, Pitch: 绕Y轴
            const float initial_roll = std::atan2(ay, az);
            const float initial_pitch = std::atan2(-ax, std::sqrt(ay * ay + az * az));
            constexpr float initial_yaw = 0.0f; // 没有磁力计，无法获知绝对航向

            // 欧拉角转四元数初始化 g_mahony.q
            // cy = cos(yaw/2), cp = cos(pitch/2), cr = cos(roll/2) ...
            const float cy = std::cos(initial_yaw * 0.5f);
            const float sy = std::sin(initial_yaw * 0.5f);
            const float cp = std::cos(initial_pitch * 0.5f);
            const float sp = std::sin(initial_pitch * 0.5f);
            const float cr = std::cos(initial_roll * 0.5f);
            const float sr = std::sin(initial_roll * 0.5f);

            g_mahony.q[0] = cr * cp * cy + sr * sp * sy; // w
            g_mahony.q[1] = sr * cp * cy - cr * sp * sy; // x
            g_mahony.q[2] = cr * sp * cy + sr * cp * sy; // y
            g_mahony.q[3] = cr * cp * sy - sr * sp * cy; // z

            LOG_DBG("Mahony initialized from gravity. R:%.2f P:%.2f", initial_roll, initial_pitch);
        }
    } // namespace

    void ImuHub::setup()
    {
        bool configured = false;
        if (!m_devs.empty())
            configured |= (configure_context(accel_ctx, m_devs[0]) == 0);
        if (m_devs.size() > 1)
            configured |= (configure_context(gyro_ctx, m_devs[1]) == 0);

        if (!configured)
        {
            LOG_ERR("No IMU devices");
            return;
        }

        start_worker_thread();

        for (auto* ctx : contexts)
        {
            if (ctx->enabled)
                submit_async_read(*ctx);
        }
        LOG_DBG("ImuHub async pipeline started");

#ifdef CONFIG_IMU_HUB_INITIAL_CALIBRATION
        LOG_INF("Starting IMU calibration (%d ms). Please keep stationary.",
                CONFIG_IMU_HUB_INIT_CALIBRATION_DURATION_MS);
#endif
    }

    void ImuHub::async_worker_thread(void* p1, void* p2, void* p3)
    {
        ARG_UNUSED(p1);
        ARG_UNUSED(p2);
        ARG_UNUSED(p3);
        while (true)
        {
            sensor_processing_with_callback(&imu_rtio_ctx, process_imu_data);
        }
    }

    void ImuHub::process_imu_data(int result, uint8_t* buf, uint32_t buf_len, void* userdata)
    {
        ARG_UNUSED(buf_len);
        auto* ctx = static_cast<AsyncSensorContext*>(userdata);
        if (ctx == nullptr || !ctx->enabled)
            return;
        if (result != 0)
        {
            submit_async_read(*ctx);
            return;
        }

        const sensor_decoder_api* decoder = nullptr;
        if (sensor_get_decoder(ctx->dev, &decoder) != 0)
        {
            submit_async_read(*ctx);
            return;
        }

        sensor_three_axis_data decoded = {};
        uint32_t fit = 0;
        const sensor_chan_spec channel = {.chan_type = static_cast<uint16_t>(ctx->channel_type), .chan_idx = 0};

        if (decoder->decode(buf, channel, &fit, 1, &decoded) <= 0)
        {
            submit_async_read(*ctx);
            return;
        }

        // 调用处理函数
        getInstance().handle_axis_update(ctx->channel_type, decoded);

        submit_async_read(*ctx);
        k_sleep(K_TICKS(10));
    }

    std::function<void(IMUData&)> update_func;

    void ImuHub::handle_axis_update(const sensor_channel channel,
                                    const sensor_three_axis_data& data)
    {
        auto to_float = [&](const q31_t value)
        {
            return q31_to_float(value, data.shift);
        };

        const IMUData::Vector3 vec{
            to_float(data.readings[0].x),
            to_float(data.readings[0].y),
            to_float(data.readings[0].z),
        };

        // 1. 更新 Raw Data 到全局结构体 (即使在校准时，Accel数据也是需要的)
        if (channel == SENSOR_CHAN_ACCEL_XYZ)
        {
            g_imu_data.accel = vec;
        }
        else if (channel == SENSOR_CHAN_GYRO_XYZ)
        {
            g_imu_data.gyro = vec;
        }

#ifdef CONFIG_IMU_HUB_INITIAL_CALIBRATION
        if (!g_is_calibrated)
        {
            // 初始化开始时间
            if (g_calib_start_time < 0)
            {
                g_calib_start_time = k_uptime_get();
            }

            // 仅对陀螺仪进行积分
            if (channel == SENSOR_CHAN_GYRO_XYZ)
            {
                g_gyro_accum.x += vec.x;
                g_gyro_accum.y += vec.y;
                g_gyro_accum.z += vec.z;
                g_calib_sample_count++;
            }

            // 检查时间是否到达
            const int64_t now = k_uptime_get();
            if ((now - g_calib_start_time) >= CONFIG_IMU_HUB_INIT_CALIBRATION_DURATION_MS)
            {
                if (g_calib_sample_count > 0)
                {
                    // 计算 Bias
                    g_gyro_bias.x = static_cast<float>(g_gyro_accum.x / g_calib_sample_count);
                    g_gyro_bias.y = static_cast<float>(g_gyro_accum.y / g_calib_sample_count);
                    g_gyro_bias.z = static_cast<float>(g_gyro_accum.z / g_calib_sample_count);

                    LOG_INF("Calibration Done. Samples: %d, Bias: %.4f, %.4f, %.4f",
                            g_calib_sample_count, g_gyro_bias.x, g_gyro_bias.y, g_gyro_bias.z);

                    // 重置时间戳，防止校准结束后第一帧计算出巨大的 dt
                    g_prev_timestamp = 0;

                    // 利用当前加速度计数据，强行对齐 Mahony 的姿态
                    // 这样系统一开始就是平的，不需要花时间收敛
                    auto [ax, ay, az] = g_imu_data.accel;
                    // 只有当加速度模长接近 1g (9.8 +/- 容差) 时才对齐，这里简化处理直接对齐
                    align_mahony_to_gravity(ax, ay, az);
                }
                else
                {
                    LOG_WRN("Calibration finished but no gyro samples received!");
                }

                // 标记校准完成，允许后续逻辑执行
                g_is_calibrated = true;
            }

            // 校准期间不运行 Mahony，直接返回
            // 此时 g_imu_data.quat 保持默认值或者全0，外部消费者可以通过四元数是否为0判断未就绪
            return;
        }
#endif

        // 3. 应用 Bias 修正
        auto [ax, ay, az] = g_imu_data.accel;
        auto [gx, gy, gz] = g_imu_data.gyro;

#ifdef CONFIG_IMU_HUB_INITIAL_CALIBRATION
        gx -= g_gyro_bias.x;
        gy -= g_gyro_bias.y;
        gz -= g_gyro_bias.z;
        g_imu_data.gyro = {gx, gy, gz};
#endif

        // 速度计死区
        if constexpr (constexpr uint64_t CONFIG_GYRO_DEADBAND = CONFIG_IMU_HUB_GYRO_DEADBAND; CONFIG_GYRO_DEADBAND > 0)
        {
            constexpr float GYRO_DEADBAND = static_cast<float>(CONFIG_IMU_HUB_GYRO_DEADBAND) / 1000;

            if (std::abs(gx) < GYRO_DEADBAND)
                gx = 0.0f;
            if (std::abs(gy) < GYRO_DEADBAND)
                gy = 0.0f;
            if (std::abs(gz) < GYRO_DEADBAND)
                gz = 0.0f;
        }

        const uint64_t timestamp = data.header.base_timestamp_ns;
        if (g_prev_timestamp == 0)
        {
            g_prev_timestamp = timestamp;
            return;
        }

        if (timestamp <= g_prev_timestamp) // timestamp溢出后重置全局时间辍
        {
            g_prev_timestamp = timestamp;
            return;
        }

        const uint64_t delta_ns = timestamp - g_prev_timestamp;
        g_prev_timestamp = timestamp;
        const float dt = static_cast<float>(delta_ns) * 1e-9f;

        // 4. Mahony 更新
        g_mahony.update(gx, gy, gz, ax, ay, az, dt);


        if (g_cnt++ < UPDATE_COUNT)
            return;
        g_cnt = 0;

        // 5. 发布数据
        g_imu_data.quat = {g_mahony.q[0], g_mahony.q[1], g_mahony.q[2], g_mahony.q[3]};
        auto& [p, r, y] = g_imu_data.euler_angle;
        g_mahony.getEulerAngle(p, r, y);

        updateData(g_imu_data);
    }

    ImuHub::ImuHub() = default;
} // namespace OF
