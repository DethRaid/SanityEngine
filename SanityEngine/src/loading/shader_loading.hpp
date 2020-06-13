#pragma once

#include <rx/core/vector.h>

namespace Rx {
    struct String;
}

Rx::Vector<uint8_t> load_shader(const Rx::String& shader_filename);
