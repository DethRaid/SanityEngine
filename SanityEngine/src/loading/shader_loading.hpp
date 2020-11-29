#pragma once

#include "core/types.hpp"
#include "rx/core/vector.h"

namespace Rx {
    struct String;
}

namespace sanity::engine {
    Rx::Vector<Uint8> load_shader(const Rx::String& shader_filename);
}
