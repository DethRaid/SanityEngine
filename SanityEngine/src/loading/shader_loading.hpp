#pragma once

#include "core/types.hpp"
#include "rx/core/vector.h"

namespace Rx {
    struct String;
}

Rx::Vector<Uint8> load_shader(const Rx::String& shader_filename);
