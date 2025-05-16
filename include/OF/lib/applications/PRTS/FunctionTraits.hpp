// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#ifndef FUNCTIONTRAITS_HPP
#define FUNCTIONTRAITS_HPP

#include <tuple>

template <typename T>
struct FunctionTraits;

template <typename R, typename C, typename... Args>
struct FunctionTraits<R(C::*)(Args...)>
{
    using ClassType = C;
    using ReturnType = R;
    using ArgTuple = std::tuple<Args...>;
    static constexpr size_t Arity = sizeof...(Args);
};


#endif //FUNCTIONTRAITS_HPP
