#ifndef VTHUB_HPP
#define VTHUB_HPP

#include <zephyr/kernel.h>
#include <tl/expected.hpp>
#include <cstdint>

namespace OF {

struct VtHubError {
    enum class Code {
        DISCONNECTED,
    } code;
    const char* message;
};

class VtHub {
public:
    static constexpr auto name = "VtHub";

    /**
     * @brief Get the latest received packet of type T.
     * 
     * @tparam T The packet type to retrieve.
     * @return tl::expected<T, VtHubError> containing the packet or an error.
     */
    template <typename T>
    static tl::expected<T, VtHubError> get();

private:
    // Helper to check connection status
    static bool is_connected();
};

} // namespace OF

#endif // VTHUB_HPP
