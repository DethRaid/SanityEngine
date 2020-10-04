#pragma once
#include "rx/core/string.h"

#include "nlohmann/json.hpp"
#include "RexWrapper.hpp"

using nlohmann::json;

namespace Sanity::Codegen 
{
	struct CompilationDatabaseEntry
	{
        Rx::String directory{};

        Rx::String file{};

		Rx::Vector<Rx::String> arguments{};

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(CompilationDatabaseEntry, directory, file, arguments);
	};
}


