#include "shader_loading.hpp"

#include <cstdio> // fopen, fseek, ftell, rewind, fread, fclose, SEEK_END

#include "globals.hpp"
#include "rx/core/filesystem/file.h"
#include "rx/core/log.h"
#include "rx/core/rex_string.h"
#include "sanity_engine.hpp"

RX_LOG("ShaderLoading", logger);

Rx::Vector<Uint8> load_shader(const Rx::String& shader_filename) {
    const auto& exe_directory = g_engine->executable_directory;

    const auto shader_filepath = Rx::String::format("%s/data/shaders/%s", exe_directory, shader_filename);

    const auto file_data = Rx::Filesystem::read_binary_file(shader_filepath);

    if(!file_data) {
        logger->error("Could not open shader file '%s'", shader_filepath);
        return {};
    }

    return *file_data;
}
