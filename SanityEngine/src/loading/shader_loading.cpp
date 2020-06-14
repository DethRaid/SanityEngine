#include "shader_loading.hpp"

#include <minitrace.h>
#include <rx/core/string.h>
#include <spdlog/spdlog.h>

Rx::Vector<uint8_t> load_shader(const Rx::String& shader_filename) {
    const auto shader_filepath = fmt::format("data/shaders/{}", shader_filename.data());

    auto* shader_file = fopen(shader_filepath.c_str(), "rb");
    if(shader_file == nullptr) {
        spdlog::error("Could not open shader file '{}'", shader_filepath);
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
