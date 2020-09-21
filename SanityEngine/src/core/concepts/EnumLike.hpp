#pragma once

#include "core/types.hpp"

template <typename T>
concept EnumLike = requires(T a) {
    {static_cast<Uint32>(a)};
};
