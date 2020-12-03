#pragma once

#include "RexWrapper.hpp"
#include "nlohmann/json_fwd.hpp"
#include "rx/core/string.h"

using nlohmann::json;

namespace Sanity::Codegen {
    struct CompilationDatabaseEntry {
        Rx::String directory{};

        Rx::String file{};

        Rx::Vector<Rx::String> arguments{};

        // NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CompilationDatabaseEntry, directory, file, arguments);
    };

    // ReSharper disable CppInconsistentNaming
    void to_json(json& j, const CompilationDatabaseEntry& entry);

    void from_json(const json& j, CompilationDatabaseEntry& entry);
    // ReSharper restore CppInconsistentNaming
} // namespace Sanity::Codegen
