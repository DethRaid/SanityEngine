#include "shader_loading.hpp"

#include <cstdio>

#include <minitrace.h>
#include <rx/core/log.h>
#include <rx/core/string.h>

RX_LOG("ShaderLoading", logger);

Rx::Vector<uint8_t> load_shader(const Rx::String& shader_filename) {
    const auto shader_filepath = Rx::String::format("data/shaders/%s", shader_filename);

    auto* shader_file = fopen(shader_filepath.data(), "rb");
    if(shader_file == nullptr) {
        logger->error("Could not open shader file '{}'", shader_filepath);
        return {};
    }

    fseek(shader_file, 0, SEEK_END);
    const auto file_size = ftell(shader_file);

    auto shader = Rx::Vector<uint8_t>(file_size);

    rewind(shader_file);

    fread(shader.data(), sizeof(uint8_t), file_size, shader_file);
    fclose(shader_file);

    return shader;
}
