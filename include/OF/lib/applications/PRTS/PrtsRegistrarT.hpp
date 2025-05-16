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

            auto handler = [unitName, cmdName, desc](const struct shell* sh, int argc, char** argv) -> int
            {
                std::vector<std::string_view> argvec;
                for (int i = 1; i < argc; ++i)
                {
                    argvec.push_back(argv[i]);
                }
                ArgTup args{};
                if (!ArgParser<0, ArgTup>::parse(args, argvec))
                {
                    shell_error(sh, "Missing or invalid options, try prts help [unit] [cmd]");
                    return -EINVAL;
                }
                auto uopt = UnitRegistry::findUnit(unitName);
                if (!uopt)
                {
                    shell_error(sh, "Unit '%s' not found", unitName.data());
                    return -ENOENT;
                }
                C* u = static_cast<C*>(*uopt);
                return std::apply(
                    [u]<typename... T>(T&&... a)
                    {
                        return (u->*MemFn)(std::forward<T>(a)...);
                    },
                    args);
            };

            const CommandDesc cd = {
                .unitName = unitName,
                .cmdName = cmdName,
                .description = desc,
                .options = std::move(opts),
                .handler = handler
            };
            PrtsManager::registerCommand(cd);
        }
    };
}

#endif //PRTSREGISTRART_HPP
