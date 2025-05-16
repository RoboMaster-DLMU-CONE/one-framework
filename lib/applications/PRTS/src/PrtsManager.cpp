// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <zephyr/shell/shell.h>
#include <OF/lib/applications/PRTS/PrtsManager.hpp>

#include <string>
#include <unordered_set>

#include "OF/lib/applications/Unit/UnitRegistry.hpp"

using namespace OF::Prts;
using enum OptionType;


// prts ls unit|cmd
static int cmd_ls(const shell* sh, size_t argc, char** argv)
{
    if (argc < 2)
    {
        shell_error(sh, "Usage: prts ls unit|cmd");
        return -EINVAL;
    }
    const std::string arg = argv[1];
    if (arg == "unit")
    {
        std::unordered_set<std::string_view> names;
        for (auto& cd : PrtsManager::getCommands())
        {
            names.insert(cd.unitName);
        }
        for (auto& n : names)
        {
            shell_print(sh, "%s", n.data());
        }
        return 0;
    }
    if (arg == "cmd")
    {
        if (argc < 3)
        {
            shell_error(sh, "Usage: prts ls cmd <unit>");
            return -EINVAL;
        }
        std::string_view unit = argv[2];
        for (auto& cd : PrtsManager::getCommands())
        {
            if (cd.unitName == unit)
            {
                shell_print(sh, "%s", cd.cmdName.data());
            }
        }
        return 0;
    }
    shell_error(sh, "Unknown ls target '%s'", arg.c_str());
    return -EINVAL;
}

static int cmd_info(const struct shell* sh, size_t argc, char** argv)
{
    if (argc != 2)
    {
        shell_error(sh, "Usage: prts info <unit>");
        return -EINVAL;
    }
    const std::string_view unit = argv[1];
    const auto opt = OF::UnitRegistry::findUnit(unit);
    if (!opt)
    {
        shell_error(sh, "Unit '%s' not found", std::string(unit).c_str());
        return -ENOENT;
    }
    OF::Unit* u = *opt;
    shell_print(sh, "Name: %s", u->getName().data());
    shell_print(sh, "Desc: %s", u->getDescription().data());
    shell_print(sh, "StackSize: %zu", u->getStackSize());
    shell_print(sh, "Priority: %u", u->getPriority());
    shell_print(sh, "State: %d", static_cast<int>(u->state));
    shell_print(sh, "CPU: %u%%  Mem: %u bytes", u->stats.cpuUsage, u->stats.memoryUsage);
    return 0;
}

// prts call <unit> <cmd> [--opt v …]
static int cmd_call(const shell* sh, size_t argc, char** argv)
{
    if (argc < 3)
    {
        shell_error(sh, "Usage: prts call <unit> <cmd> [options]");
        return -EINVAL;
    }
    const std::string_view unit = argv[1];
    const std::string_view cmd = argv[2];
    for (const auto& cd : PrtsManager::getCommands())
    {
        if (cd.unitName == unit && cd.cmdName == cmd)
        {
            // 注意 handler 的 argc, argv 都从 cmd 名开始
            return cd.handler(sh, static_cast<int>(argc - 2), &argv[2]);
        }
    }
    shell_error(sh, "未在 Unit '%s' 中找到 '%s' 命令。", unit.data(), cmd.data());
    return -ENOENT;
}

static int cmd_elements(const shell* sh, size_t argc, char** argv)
{
    if (argc != 2)
    {
        shell_error(sh, "Usage: prts elements <unit>");
        return -EINVAL;
    }
    const std::string_view name = argv[1];
    auto uopt = OF::UnitRegistry::findUnit(name);
    if (!uopt)
    {
        shell_error(sh, "Unit '%s' not found", name.data());
    }
    const OF::Unit* u = *uopt;

    for (auto& ed : PrtsManager::getElements())
    {
        if (ed.unitName == name)
        {
            std::string_view val = ed.getter(u);
            shell_print(sh, "%s [%s] = %s (min=%g max=%g)",
                        ed.elemName.data(),
                        ed.type.data(),
                        val.data(),
                        ed.minVal,
                        ed.maxVal);
        }
    }
    return 0;
}

static int cmd_help(const struct shell* sh, size_t argc, char** argv)
{
    if (argc != 3)
    {
        shell_error(sh, "Usage: prts help <unit> <cmd>");
        return -EINVAL;
    }
    const std::string_view unit = argv[1];
    const std::string_view cmd = argv[2];
    for (const auto& cd : PrtsManager::getCommands())
    {
        if (cd.unitName == unit && cd.cmdName == cmd)
        {
            shell_print(sh, "Usage: prts call %s %s", unit.data(), cmd.data());
            shell_print(sh, "  %s", cd.description.data());
            for (const auto& [name, type] : cd.options)
            {
                const char* t = type == INT
                    ? "int"
                    : type == DOUBLE
                    ? "double"
                    : "string";
                shell_print(sh, "    --%s <%s>", name.data(), t);
            }
            return 0;
        }
    }
    shell_error(sh, "Command '%s' not found for unit '%s'", cmd.data(), unit.data());
    return -ENOENT;
}

SHELL_STATIC_SUBCMD_SET_CREATE(prts_subcmds,
                               SHELL_CMD(help, NULL, "Show command help", cmd_help),
                               SHELL_CMD(ls, NULL, "List units or commands", cmd_ls),
                               SHELL_CMD(info, NULL, "Show unit info", cmd_info),
                               SHELL_CMD(call, NULL, "Invoke a unit command",cmd_call),
                               SHELL_CMD(elements, NULL, "List UI elements", cmd_elements),
                               SHELL_SUBCMD_SET_END
    );

void PrtsManager::initShell()
{
    SHELL_CMD_REGISTER(prts, &prts_subcmds, "PRTS command group", NULL);
}
