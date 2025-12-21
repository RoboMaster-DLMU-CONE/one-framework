#ifndef OF_FIXEDSTRING_HPP
#define OF_FIXEDSTRING_HPP

#include <assert.h>
#include <cstring>
#include <string>

namespace OF
{
    template <size_t Size>
    class FixedString
    {
    public:
        FixedString() { data[0] = '\0'; }

        FixedString(const char* str)
        {
            assert(str != nullptr);
            const auto len = strlen(str);
            assert(len <= Size);
            memcpy(data, str, len);
            data[len] = '\0';
        }

        FixedString(const std::string& str) : FixedString(str.c_str())
        {
        }

        bool operator==(const FixedString& other) const
        {
            return strcmp(data, other.data) == 0;
        }

        std::string_view view() const
        {
            return std::string_view(data);
        }

    private:
        char data[Size];
    };
}

template <size_t Size>
struct std::hash<OF::FixedString<Size>>
{
    size_t operator()(const OF::FixedString<Size>& k) const
    {
        return hash<string_view>{}(k.view());
    }
};

#endif //OF_FIXEDSTRING_HPP
