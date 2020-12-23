#include "path_ops.hpp"

#include <filesystem>

namespace sanity::engine {
    fs::path engine::append_extension(const fs::path& path, const fs::path& extension) {
        const auto path_string = path.string();
        const auto extension_string = extension.string();
        auto* extension_c_str = extension_string.c_str();
        if(*extension_c_str == '.') {
            ++extension_c_str;
        }

    	return path_string + '.' + extension_c_str;
    }
} // namespace sanity::engine