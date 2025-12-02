#include <OF/lib/ImuHub/ImuHub.hpp>

#include <errno.h>
#include <math.h>

#include <zephyr/logging/log.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(ImuHub, CONFIG_IMU_HUB_LOG_LEVEL);

namespace OF
{
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

        sensor_chan_spec accel_channels[] = {
            {SENSOR_CHAN_ACCEL_XYZ, 0},
        };

        sensor_chan_spec gyro_channels[] = {
            {SENSOR_CHAN_GYRO_XYZ, 0},
        };

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

        RTIO_IODEV_DEFINE(imu_accel_iodev, &__sensor_iodev_api, &accel_read_cfg);
        RTIO_IODEV_DEFINE(imu_gyro_iodev, &__sensor_iodev_api, &gyro_read_cfg);

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

        K_THREAD_STACK_DEFINE(imu_rtio_stack, CONFIG_IMU_HUB_RTIO_STACK_SIZE);
        k_thread imu_rtio_thread_data;
        bool imu_rtio_thread_started;

        constexpr int kRtioThreadPriority =
            (CONFIG_IMU_HUB_RTIO_THREAD_PRIORITY == 0)
            ? K_LOWEST_APPLICATION_THREAD_PRIO
            : CONFIG_IMU_HUB_RTIO_THREAD_PRIORITY;

        static_assert(kRtioThreadPriority >= K_HIGHEST_APPLICATION_THREAD_PRIO &&
                      kRtioThreadPriority <= K_LOWEST_APPLICATION_THREAD_PRIO,
                      "Invalid IMU Hub RTIO thread priority");

        float q31_to_float(q31_t value, int8_t shift)
        {
            return ldexpf(static_cast<float>(value), shift - 31);
        }

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

            constexpr sensor_value freq = {
                .val1 = CONFIG_IMU_HUB_SAMPLING_FREQUENCY,
                .val2 = 0,
            };

            err = sensor_attr_set(dev, SENSOR_CHAN_ALL, SENSOR_ATTR_SAMPLING_FREQUENCY, &freq);
            if (err != 0)
            {
                LOG_DBG("Unable to set sampling frequency for %s: %d", dev->name, err);
            }

            return 0;
        }

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

        start_worker_thread();

        for (auto* ctx : contexts)
        {
            if (ctx->enabled)
            {
                submit_async_read(*ctx);
            }
        }

        LOG_DBG("ImuHub async pipeline started");
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
        {
            LOG_ERR("Async sensor context missing");
            return;
        }

        if (result != 0)
        {
            LOG_ERR("Async read failed for %s: %d", ctx->dev->name, result);
            submit_async_read(*ctx);
            return;
        }

        const sensor_decoder_api* decoder = nullptr;
        int rc = sensor_get_decoder(ctx->dev, &decoder);

        if (rc != 0)
        {
            LOG_ERR("sensor_get_decoder(%s) failed: %d", ctx->dev->name, rc);
            submit_async_read(*ctx);
            return;
        }

        sensor_three_axis_data decoded = {};
        uint32_t fit = 0;
        sensor_chan_spec channel = {
            .chan_type = ctx->channel_type,
            .chan_idx = 0,
        };

        rc = decoder->decode(buf, channel, &fit, 1, &decoded);
        if (rc <= 0)
        {
            LOG_WRN("decoder->decode(%s) returned %d", ctx->dev->name, rc);
            submit_async_read(*ctx);
            return;
        }

        getInstance().handle_axis_update(ctx->channel_type, decoded);

        submit_async_read(*ctx);
        k_sleep(K_TICKS(10));
    }

    void ImuHub::handle_axis_update(const sensor_channel channel,
                                    const sensor_three_axis_data& data)
    {
        auto to_float = [&](q31_t value)
        {
            return q31_to_float(value, data.shift);
        };

        IMUData::Vector3 vec{
            to_float(data.readings[0].x),
            to_float(data.readings[0].y),
            to_float(data.readings[0].z),
        };

        manipulateData([&](IMUData& imu, void* _)
        {
            if (channel == SENSOR_CHAN_ACCEL_XYZ)
            {
                imu.accel = vec;
            }
            else if (channel == SENSOR_CHAN_GYRO_XYZ)
            {
                imu.gyro = vec;
            }
        }, nullptr);

        LOG_DBG("IMU %s updated: (%.3f, %.3f, %.3f)",
                channel == SENSOR_CHAN_ACCEL_XYZ ? "accel" : "gyro",
                vec.x, vec.y, vec.z);
    }

    ImuHub::ImuHub() = default;
} // namespace OF
