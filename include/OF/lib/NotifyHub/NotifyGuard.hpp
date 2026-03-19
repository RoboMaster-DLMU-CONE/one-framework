#ifndef OF_NOTIFY_GUARD_HPP
#define OF_NOTIFY_GUARD_HPP

#include <OF/lib/NotifyHub/NotifyHub.hpp>
#include <optional>

namespace OF
{
    /**
     * @brief 状态去重 Guard，只在状态真正改变时才发送通知
     * 
     * @tparam StatusT LEDStatus 或 BuzzerStatus
     * 
     * 使用示例:
     *   NotifyGuard<LEDStatus> m_led_guard{"chassis"};
     *   m_led_guard.set({c_warning, LEDMode::Breathing, 1, 300}); // 自动去重
     */
    template<typename StatusT>
    class NotifyGuard
    {
    public:
        explicit NotifyGuard(const FixedString<MAX_KEY_LEN>& key) : m_key(key) {}

        /**
         * @brief 设置状态，只有与上次不同时才发送
         * @param status 新状态
         * @return true 如果实际发送了更新
         */
        bool set(const StatusT& status)
        {
            if (m_last_status.has_value() && m_last_status.value() == status)
            {
                return false;
            }
            m_last_status = status;
            sendStatus(status);
            return true;
        }

        /**
         * @brief 重置缓存，下次 set 将强制发送
         */
        void reset()
        {
            m_last_status.reset();
        }

        /**
         * @brief 获取当前缓存的状态（如果有）
         */
        const std::optional<StatusT>& current() const
        {
            return m_last_status;
        }

    private:
        FixedString<MAX_KEY_LEN> m_key;
        std::optional<StatusT> m_last_status;

        void sendStatus(const StatusT& status);
    };

    // 特化 LEDStatus 的发送逻辑
    template<>
    inline void NotifyGuard<LEDStatus>::sendStatus(const LEDStatus& status)
    {
        NotifyHub::setLEDStatus(m_key, status);
    }

    // 特化 BuzzerStatus 的发送逻辑
    template<>
    inline void NotifyGuard<BuzzerStatus>::sendStatus(const BuzzerStatus& status)
    {
        NotifyHub::setBuzzerStatus(m_key, status);
    }
}

#endif // OF_NOTIFY_GUARD_HPP
