#pragma once

#include <filesystem>

namespace fs = std::filesystem;

namespace sanity::engine {
    fs::path append_extension(const fs::path& path, const fs::path& extension);
}
