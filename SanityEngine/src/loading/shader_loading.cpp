#include "shader_loading.hpp"

#include <cstdio> // fopen, fseek, ftell, rewind, fread, fclose, SEEK_END

#include "rx/core/log.h"
#include "rx/core/string.h"
#include "sanity_engine.hpp"

RX_LOG("ShaderLoading", logger);

Rx::Vector<Uint8> load_shader(const Rx::String& shader_filename) {
    const auto& exe_directory = SanityEngine::executable_directory;

    const auto shader_filepath = Rx::String::format("%s/data/shaders/%s", exe_directory, shader_filename);

    auto* shader_file = fopen(shader_filepath.data(), "rb");
    if(shader_file == nullptr) {
        logger->error("Could not open shader file '%s'", shader_filepath);
        return {};
    }

    fseek(shader_file, 0, SEEK_END);
    const auto file_size = ftell(shader_file);

    auto shader = Rx::Vector<Uint8>{static_cast<Size>(file_size)};

    rewind(shader_file);

    fread(shader.data(), sizeof(Uint8), file_size, shader_file);
    fclose(shader_file);

    return shader;
}
