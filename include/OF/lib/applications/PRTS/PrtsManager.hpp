// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#ifndef PRTSMANAGER_HPP
#define PRTSMANAGER_HPP
#include <string_view>
#include <vector>
#include <functional>

namespace OF::Prts
{
    enum class OptionType { INT, DOUBLE, STRING };

    struct OptionDesc
    {
        std::string_view name;
        OptionType type;
    };

    struct CommandDesc
    {
        std::string_view unitName;
        std::string_view cmdName;
        std::string_view description;
        std::vector<OptionDesc> options;
        std::function<int(int, char**)> handler;
    };

    struct ElementDesc
    {
        std::string_view unitName;
        std::string_view elemName;
        std::string_view type; // "button","slider","text"
        double minVal;
        double maxVal;
    };

    class PrtsManager
    {
    public:
        static void registerCommand(const CommandDesc& d) { getCommands().push_back(d); };

        static void registerElement(const ElementDesc& e) { getElements().push_back(e); };
        static void initShell();

        static std::vector<CommandDesc>& getCommands()
        {
            static std::vector<CommandDesc> cmds;
            return cmds;
        };

        static std::vector<ElementDesc>& getElements()
        {
            static std::vector<ElementDesc> elems;
            return elems;
        };
    };
}

#endif //PRTSMANAGER_HPP
