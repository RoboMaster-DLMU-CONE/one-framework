// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#ifndef PRTSREGISTRAR_HPP
#define PRTSREGISTRAR_HPP
#include "PrtsRegistrarT.hpp"

#define PRTS_COMMAND_T(UnitClass, MemFn, Desc, OptsT) \
static OF::Prts::CommandRegistrarT<&UnitClass::MemFn, OptsT> \
CONCAT(prts_reg_, MemFn){#UnitClass, #MemFn, Desc};

#define PRTS_ELEMENT_T(UnitClass, ElemName, TypeStr, MinV, MaxV, GetterFn)    \
    static OF::Prts::ElementDesc                                              \
    CONCAT(_prts_elem_desc_,ElemName) {                                        \
        #UnitClass, #ElemName, TypeStr, MinV, MaxV,                             \
        /* getter lambda */                                                      \
        [](const OF::Unit* u) -> std::string {                                   \
            auto* c = static_cast<const UnitClass*>(u);                          \
            return std::to_string(c->GetterFn());                                \
        }                                                                       \
    };                                                                          \
    static int CONCAT(_prts_elem_reg_,ElemName) =                               \
    (OF::Prts::PrtsManager::registerElement(CONCAT(_prts_elem_desc_,ElemName)), 0);

#endif //PRTSREGISTRAR_HPP
