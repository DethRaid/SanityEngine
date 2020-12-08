#pragma once


#include "nlohmann/json.hpp"
#include "core/RexJsonConversion.hpp"
#include "project/project_definition.hpp"

namespace sanity::editor {
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Project, name);
}
