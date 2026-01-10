#ifndef OF_REMAP_HPP
#define OF_REMAP_HPP

namespace OF
{
    template <float IN_MIN, float IN_MAX, float OUT_MIN, float OUT_MAX>
    [[nodiscard]] float remap(const float x)
    {
        return (x - IN_MIN) * (OUT_MAX - OUT_MIN) / (IN_MAX - IN_MIN) + OUT_MIN;
    }
}

#endif //OF_REMAP_HPP