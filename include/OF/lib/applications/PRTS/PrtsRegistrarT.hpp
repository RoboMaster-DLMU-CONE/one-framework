#ifndef PRTSREGISTRART_HPP
#define PRTSREGISTRART_HPP

#include "FunctionTraits.hpp"
#include "ArgParser.hpp"
#include "PrtsManager.hpp"
#include <utility>

#include "OF/lib/applications/Unit/UnitRegistry.hpp"

namespace OF::Prts
{

    template <auto MemFn, typename OptsVec>
    struct CommandRegistrarT
    {
        CommandRegistrarT(std::string_view unitName,
                          std::string_view cmdName,
                          std::string_view desc
            )
        {
            using Traits = FunctionTraits<decltype(MemFn)>;
            using C = typename Traits::ClassType;
            using ArgTup = typename Traits::ArgTuple;

            std::vector<OptionDesc> opts(
                OptsVec::value.begin(),
                OptsVec::value.end()
                );

            const CommandDesc cd = {
                .unitName = unitName,
                .cmdName = cmdName,
                .description = desc,
                .options = std::move(opts),
                // ReSharper disable once CppParameterMayBeConst
                .handler = [unitName](int argc, char** argv)-> int
                {
                    auto uopt = UnitRegistry::findUnit(unitName);
                    if (!uopt)
                        return -1;
                    C* u = static_cast<C*>(*uopt);

                    std::vector<std::string_view> argvec;
                    for (int i = 1; i < argc; ++i)
                    {
                        argvec.push_back(argv[i]);
                    }
                    ArgTup args{};
                    if (!ArgParser<0, ArgTup>::parse(args, argvec))
                    {
                        return -1;
                    }
                    return std::apply([u]<typename... T>(T&&... a)
                    {
                        return (u->*MemFn)(std::forward<T>(a)...);
                    }, args);
                }
            };
            PrtsManager::registerCommand(cd);
        }
    };
}

#endif //PRTSREGISTRART_HPP
