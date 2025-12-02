#include <OF/lib/ImuHub/ImuHub.hpp>
#include <OF/utils/Mahony.hpp>
#include <OF/utils/CCM.h>

#include <cerrno>
#include <cmath>

#include <zephyr/logging/log.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(ImuHub, CONFIG_IMU_HUB_LOG_LEVEL);

OF_CCM_ATTR uint8_t g_cnt{};
constexpr uint16_t UPDATE_COUNT = CONFIG_IMU_HUB_PUBLISH_EVERY_N_FRAME / 2 * 2;
BUILD_ASSERT(CONFIG_IMU_HUB_PUBLISH_EVERY_N_FRAME != 0, "CONFIG_IMU_HUB_DATA_UPDATE_FRAME_CNT should not be 0!");


namespace OF
{
    OF_CCM_ATTR Mahony g_mahony{};
    OF_CCM_ATTR IMUData g_imu_data{};
    OF_CCM_ATTR uint64_t g_prev_timestamp{};
    RTIO_DEFINE_WITH_MEMPOOL(imu_rtio_ctx, 16, 16, 16, 512, sizeof(void *));

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

        // Channel spec arrays define which sensor channels we request from device
        sensor_chan_spec accel_channels[] = {
            {SENSOR_CHAN_ACCEL_XYZ, 0},
        };

        sensor_chan_spec gyro_channels[] = {
            {SENSOR_CHAN_GYRO_XYZ, 0},
        };

        // Read configs used by the RTIO I/O device wrappers. These point at the
        // channel spec arrays above and configure streaming behavior.
        sensor_read_config accel_read_cfg = {
            .sensor = nullptr,
            .is_streaming = false,
            .channels = accel_channels,
            .count = std::size(accel_channels),
            .max = std::size(accel_channels),
        };

        sensor_read_config gyro_read_cfg = {
            .sensor = nullptr,
            .is_streaming = false,
            .channels = gyro_channels,
            .count = std::size(gyro_channels),
            .max = std::size(gyro_channels),
        };

        // Define two RTIO I/O device wrappers, one for accel and one for gyro.
        RTIO_IODEV_DEFINE(imu_accel_iodev, &__sensor_iodev_api, &accel_read_cfg);
        RTIO_IODEV_DEFINE(imu_gyro_iodev, &__sensor_iodev_api, &gyro_read_cfg);

        // Per-context state used when submitting async reads and handling completions.
        AsyncSensorContext accel_ctx = {
            .iodev = &imu_accel_iodev,
            .channels = accel_channels,
            .channel_count = std::size(accel_channels),
            .channel_type = SENSOR_CHAN_ACCEL_XYZ,
        };

        AsyncSensorContext gyro_ctx = {
            .iodev = &imu_gyro_iodev,
            .channels = gyro_channels,
            .channel_count = std::size(gyro_channels),
            .channel_type = SENSOR_CHAN_GYRO_XYZ,
        };

        constexpr AsyncSensorContext* const contexts[] = {&accel_ctx, &gyro_ctx};

        // RTIO worker thread stack + thread control object.
        K_THREAD_STACK_DEFINE(imu_rtio_stack, CONFIG_IMU_HUB_RTIO_STACK_SIZE);
        k_thread imu_rtio_thread_data;
        bool imu_rtio_thread_started;

        // Thread priority validation - ensure configured priority is within
        // application thread bounds.
        constexpr int kRtioThreadPriority =
            (CONFIG_IMU_HUB_RTIO_THREAD_PRIORITY == 0)
            ? K_LOWEST_APPLICATION_THREAD_PRIO
            : CONFIG_IMU_HUB_RTIO_THREAD_PRIORITY;

        static_assert(kRtioThreadPriority >= K_HIGHEST_APPLICATION_THREAD_PRIO &&
                      kRtioThreadPriority <= K_LOWEST_APPLICATION_THREAD_PRIO,
                      "Invalid IMU Hub RTIO thread priority");

        // Helper: convert q31 fixed-point samples into float using shift value.
        float q31_to_float(q31_t value, int8_t shift)
        {
            return ldexpf(static_cast<float>(value), shift - 31);
        }

        // Bind a Zephyr sensor device to an AsyncSensorContext and configure
        // sampling frequency. Returns 0 on success or negative errno.
        int configure_context(AsyncSensorContext& ctx, const device* dev)
        {
            if (dev == nullptr)
            {
                return -ENODEV;
            }

            ctx.dev = dev;
            ctx.enabled = true;

            int err = sensor_reconfigure_read_iodev(ctx.iodev, dev, ctx.channels, ctx.channel_count);

            if (err != 0)
            {
                LOG_ERR("Failed to bind %s to channel %d: %d", dev->name, ctx.channel_type, err);
                ctx.enabled = false;
                return err;
            }

            // Try to set the sensor sampling frequency; non-fatal if it fails.
            constexpr sensor_value freq = {
                .val1 = CONFIG_IMU_HUB_SAMPLING_FREQUENCY,
                .val2 = 0,
            };

            err = sensor_attr_set(dev, SENSOR_CHAN_ALL, SENSOR_ATTR_SAMPLING_FREQUENCY, &freq);
            if (err != 0)
            {
                LOG_WRN("Unable to set sampling frequency for %s: %d", dev->name, err);
            }

            return 0;
        }

        // Submit an asynchronous read request using the RTIO mempool-backed API.
        // Returns 0 on success or negative errno.
        int submit_async_read(AsyncSensorContext& ctx)
        {
            if (!ctx.enabled)
            {
                return -EACCES;
            }

            int err = sensor_read_async_mempool(ctx.iodev, &imu_rtio_ctx, &ctx);

            if (err != 0)
            {
                LOG_ERR("sensor_read_async_mempool(%s) failed: %d", ctx.dev->name, err);
            }

            return err;
        }

        // Create and start the RTIO worker thread which processes the RTIO
        // completion queue. This thread runs sensor_processing_with_callback
        // to dispatch completed entries to the provided callback.
        void start_worker_thread()
        {
            if (imu_rtio_thread_started)
            {
                return;
            }

            k_thread_create(&imu_rtio_thread_data, imu_rtio_stack,
                            K_THREAD_STACK_SIZEOF(imu_rtio_stack),
                            ImuHub::async_worker_thread, nullptr, nullptr, nullptr,
                            kRtioThreadPriority, 0, K_NO_WAIT);
            k_thread_name_set(&imu_rtio_thread_data, "imu_rtio");
            imu_rtio_thread_started = true;
        }
    } // namespace

    void ImuHub::setup()
    {
        bool configured = false;

        if (!m_devs.empty())
        {
            configured |= (configure_context(accel_ctx, m_devs[0]) == 0);
        }

        if (m_devs.size() > 1)
        {
            configured |= (configure_context(gyro_ctx, m_devs[1]) == 0);
        }

        if (!configured)
        {
            LOG_ERR("No IMU devices configured for async pipeline");
            return;
        }

        // Start the RTIO completion processing thread once contexts are bound.
        start_worker_thread();

        // Submit the first async reads for each enabled context so the pipeline
        // becomes active.
        for (auto* ctx : contexts)
        {
            if (ctx->enabled)
            {
                submit_async_read(*ctx);
            }
        }

        LOG_DBG("ImuHub async pipeline started");
    }

    // RTOS thread entry: continuously process RTIO completions and invoke the
    // static callback `process_imu_data` for each completed read.
    void ImuHub::async_worker_thread(void* p1, void* p2, void* p3)
    {
        ARG_UNUSED(p1);
        ARG_UNUSED(p2);
        ARG_UNUSED(p3);

        while (true)
        {
            // Blocks until completions are available, then dispatches callbacks.
            sensor_processing_with_callback(&imu_rtio_ctx, process_imu_data);
        }
    }

    // RTIO completion callback. This function validates the context and result,
    // decodes the sensor buffer using the device decoder, converts fixed-point
    // data to float and updates the shared IMUData instance.
    void ImuHub::process_imu_data(int result, uint8_t* buf, uint32_t buf_len, void* userdata)
    {
        ARG_UNUSED(buf_len);

        auto* ctx = static_cast<AsyncSensorContext*>(userdata);

        if (ctx == nullptr || !ctx->enabled)
        {
            LOG_ERR("Async sensor context missing");
            return;
        }

        // Check read result and re-submit on transient failures.
        if (result != 0)
        {
            LOG_ERR("Async read failed for %s: %d", ctx->dev->name, result);
            submit_async_read(*ctx);
            return;
        }

        // Obtain sensor-specific decoder to interpret raw buffer into structured values.
        const sensor_decoder_api* decoder = nullptr;
        int rc = sensor_get_decoder(ctx->dev, &decoder);

        if (rc != 0)
        {
            LOG_ERR("sensor_get_decoder(%s) failed: %d", ctx->dev->name, rc);
            submit_async_read(*ctx);
            return;
        }

        // Prepare decoding targets.
        sensor_three_axis_data decoded = {};
        uint32_t fit = 0;
        const sensor_chan_spec channel = {
            .chan_type = static_cast<uint16_t>(ctx->channel_type),
            .chan_idx = 0,
        };

        // Decode raw buffer into a sensor_three_axis_data struct. If decoding
        // fails or returns no samples, re-submit the read and return.
        rc = decoder->decode(buf, channel, &fit, 1, &decoded);
        if (rc <= 0)
        {
            LOG_WRN("decoder->decode(%s) returned %d", ctx->dev->name, rc);
            submit_async_read(*ctx);
            return;
        }

        // Hand the decoded values to the instance method which converts q31 to
        // float and stores it inside the shared IMUData structure.
        getInstance().handle_axis_update(ctx->channel_type, decoded);

        // Re-submit another async read to continue streaming data.
        submit_async_read(*ctx);

        // Small sleep to avoid tight looping if device provides data continuously
        // and to give other threads CPU time.
        k_sleep(K_TICKS(10));
    }

    std::function<void(IMUData&)> update_func;

    // Convert decoded q31 samples to float and update the hub's shared state.
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

        if (channel == SENSOR_CHAN_ACCEL_XYZ)
        {
            g_imu_data.accel = vec;
        }
        else if (channel == SENSOR_CHAN_GYRO_XYZ)
        {
            g_imu_data.gyro = vec;
        }
        auto [ax, ay, az] = g_imu_data.accel;
        auto [gx, gy, gz] = g_imu_data.gyro;

        const uint64_t timestamp = data.header.base_timestamp_ns;
        if (g_prev_timestamp == 0)
        {
            g_prev_timestamp = timestamp;
            return;
        }

        if (timestamp <= g_prev_timestamp)
        {
            g_prev_timestamp = timestamp;
            return;
        }

        const uint64_t delta_ns = timestamp - g_prev_timestamp;
        g_prev_timestamp = timestamp;
        const float dt = static_cast<float>(delta_ns) * 1e-9f;
        g_mahony.update(gx, gy, gz, ax, ay, az, dt);

        if (g_cnt++ < UPDATE_COUNT)
            return;
        g_cnt = 0;
        g_imu_data.quat = {g_mahony.q[0], g_mahony.q[1], g_mahony.q[2], g_mahony.q[3]};
        auto& [p, r, y] = g_imu_data.euler_angle;
        g_mahony.getEulerAngle(p, r, y);
        updateData(g_imu_data);
    }

    ImuHub::ImuHub() = default;
} // namespace OF
