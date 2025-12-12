#ifndef OF_LIB_NODE_TOPIC_HPP
#define OF_LIB_NODE_TOPIC_HPP

#include <string>

#include <OF/utils/NBuf.hpp>


namespace OF
{
    template <typename T>
    concept Printable = requires
    {
        { std::declval<T>().format() } -> std::convertible_to<std::string&&>;
    };

    template <typename T>
    class Topic
    {
    public:
        void write(const T& data)
        {
            m_buf.write(data);
        }

        template <typename Func>
        void manipulate(const Func& func)
        {
            m_buf.manipulate(func);
        }

        std::optional<T> try_read()
        {
            return m_buf.try_read();
        }

        T read()
        {
            return m_buf.read();
        }

        static void print_stub(const topic_desc* desc)
        {
            auto* self = static_cast<Topic*>(desc->topic_instance);
            T val = self->read();
            printk("Topic: %-15s | Size: %d | ", desc->name, sizeof(T));
            if constexpr (Printable<T>)
            {
                printk("%s", val.format());
            }
            printk("\n");
        }

    private:
        NBuf<T, CONFIG_TOPIC_BUFFER_N> m_buf;
    };

}

#endif //OF_LIB_NODE_TOPIC_HPP
