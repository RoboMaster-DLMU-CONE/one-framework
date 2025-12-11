#ifndef OF_TRIPLEBUF_HPP
#define OF_TRIPLEBUF_HPP

namespace OF
{
    template<typename T>
    class TripleBuf
    {
        static_assert(std::is_standard_layout<T>(), "TripleBuf can only store POD Data");

    };
}

#endif //OF_TRIPLEBUF_HPP