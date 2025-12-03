#ifndef OF_LIB_IMU_HUB_HPP
#define OF_LIB_IMU_HUB_HPP

#include <OF/lib/HubManager/HubBase.hpp>

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

namespace OF
{
    /**
     * @brief Container for IMU sample data used by the hub.
     *
     * This struct holds the most common IMU outputs used by consumers: a
     * quaternion for orientation, 3-axis gyro and accelerometer readings and
     * a temperature value.
     */
    struct IMUData
    {
        /** 3D vector helper used for gyro/accel values. */
        struct Vector3
        {
            float x, y, z; /**< X/Y/Z components in SI units (m/s^2 or rad/s). */
        };

#ifndef CONFIG_IMU_HUB_RESOLVER_NONE
        /** Basic quaternion representation for orientation. */
        struct Quaternion
        {
            float w, x, y, z; /**< Unit quaternion components (w is scalar part). */
        } quat;

        struct EulerAngle
        {
            float pitch, roll, yaw;
        } euler_angle;

#endif
        Vector3 gyro; /**< Gyroscope readings (rad/s). */
        Vector3 accel; /**< Accelerometer readings (m/s^2). */
    };

    /**
     * @brief Hub that collects asynchronous IMU data and exposes it via HubBase.
     *
     * ImuHub manages one or more Zephyr sensor devices using RTIO asynchronous
     * reads. It decodes incoming sensor buffers, converts fixed-point q31
     * samples to float, and updates the shared IMU data object.
     *
     * Usage:
     * - The HubBase infrastructure calls the ImuHub constructor through the
     *   singleton pattern provided by HubBase.
     * - Call setup() after devices are registered to initialize the async
     *   pipeline.
     *
     * Note: This class is final and non-copyable.
     */
    class ImuHub final : public HubBase<ImuHub, IMUData, true>
    {
    public:
        ImuHub(const ImuHub&) = delete;
        ImuHub operator=(const ImuHub&) = delete;

        /**
         * @brief Human readable hub name used by HubManager.
         * @return Constant null-terminated string.
         */
        [[nodiscard]] const char* getName() const override
        {
            return "ImuHub";
        }

        /**
         * @brief Configure and start the asynchronous IMU pipeline.
         *
         * This will bind RTIO I/O device wrappers to registered Zephyr
         * sensor devices, start a background RTIO worker thread and submit
         * the first async read requests.
         */
        void setup();

        /**
         * @brief RTOS thread entry used for processing the RTIO completion queue.
         *
         * Signature matches Zephyr k_thread entry point: (void*, void*, void*).
         */
        static void async_worker_thread(void* p1, void* p2, void* p3);

    private:
        friend class HubBase;
        ImuHub();
        ~ImuHub() = default;

        /** Work handler invoked by the Zephyr workqueue (unused for RTIO path). */
        static void workHandler(struct k_work* work);

        /** Synchronous work loop (unused for RTIO path). */
        void workLoop();

        /**
         * @brief RTIO completion callback invoked for each async sensor read.
         *
         * @param result Result code from the async read (0 == success).
         * @param buf Pointer to the buffer containing sensor payload.
         * @param buf_len Length of the buffer in bytes.
         * @param userdata User-provided pointer (AsyncSensorContext*).
         */
        static void process_imu_data(int result, uint8_t* buf, uint32_t buf_len, void* userdata);

        /**
         * @brief Convert decoded sensor_three_axis_data and update shared IMUData.
         *
         * @param channel The sensor channel type (accel/gyro).
         * @param data Decoded fixed-point sensor_three_axis_data.
         */
        void handle_axis_update(sensor_channel channel, const struct sensor_three_axis_data& data);

        /** Process any pending completion entries queued by the RTIO context. */
        static void process_completion_queue();

        k_work_delayable m_work{}; /**< Work item used by HubBase when polling-based flow is used. */
    };
}

#endif //OF_LIB_IMU_HUB_HPP
