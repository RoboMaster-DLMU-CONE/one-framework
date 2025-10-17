#ifndef OF_COMMBRIDGE_HPP
#define OF_COMMBRIDGE_HPP

#include <RPL/Parser.hpp>
#include <RPL/Serializer.hpp>

namespace OF
{
    template <typename... Ts>
    class CommBridge
    {
    public:
        CommBridge();

    private:
        RPL::Serializer<Ts...> m_S;
        RPL::Deserializer<Ts...> m_Ds;
        RPL::Parser<Ts...> m_Parser;

    };

    template <typename... Ts>
    CommBridge<Ts...>::CommBridge() :
        m_S(), m_Ds(), m_Parser(m_Ds)
    {

    }
}

#endif //OF_COMMBRIDGE_HPP
