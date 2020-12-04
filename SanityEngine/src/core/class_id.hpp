#pragma once

#include <guiddef.h>

#include "rx/core/string.h"

namespace sanity::engine {
    [[nodiscard]] Rx::String class_name_from_guid(const GUID& class_id);
}
