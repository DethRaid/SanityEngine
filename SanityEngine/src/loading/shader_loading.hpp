#pragma once

#include <rx/core/vector.h>

#include "core/types.hpp"

namespace Rx {
    struct String;
}

Rx::Vector<Uint8> load_shader(const Rx::String& shader_filename);
