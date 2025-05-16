// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#ifndef PRTSREGISTRAR_HPP
#define PRTSREGISTRAR_HPP
#include "PrtsRegistrarT.hpp"

#define PRTS_COMMAND_T(UnitClass, MemFn, Desc, OptsT) \
static OF::Prts::CommandRegistrarT<&UnitClass::MemFn, OptsT> \
CONCAT(prts_reg_, MemFn){#UnitClass, #MemFn, Desc};

#endif //PRTSREGISTRAR_HPP
