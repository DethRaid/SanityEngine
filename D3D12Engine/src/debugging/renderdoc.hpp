#pragma once

#include <rx/core/ptr.h>

#include "renderdoc_app.h"

namespace rx {
    struct string;
}

rx::ptr<RENDERDOC_API_1_3_0> load_renderdoc(const rx::string& renderdoc_dll_path);
