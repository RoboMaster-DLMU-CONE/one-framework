#ifndef ARGPARSER_HPP
#define ARGPARSER_HPP

#include <string>

template <size_t I, typename Tuple>
struct ArgParser
{
    static bool parse(Tuple& tup, const std::vector<std::string_view>& opts)
    {
        if constexpr (I == std::tuple_size_v<Tuple>)
        {
            return true;
        }
        else
        {
            auto& [name, type] = OF::Prts::PrtsManager::getCommands().back().options[I];
            using T = std::tuple_element_t<I, Tuple>;
            for (size_t i = 0; i + 1 < opts.size(); ++i)
            {
                if (opts[i] == "--" + std::string(name))
                {
                    if constexpr (std::is_same_v<T, int>)
                    {
                        std::get<I>(tup) = std::stoi(std::string(opts[i + 1]));
                    }
                    else if constexpr (std::is_same_v<T, double>)
                    {
                        std::get<I>(tup) = std::stod(std::string(opts[i + 1]));
                    }
                    else
                        std::get<I>(tup) = opts[i + 1];
                    return ArgParser<I + 1, Tuple>::parse(tup, opts);
                }
            }
            return false;
        }
    }
};

#endif //ARGPARSER_HPP
