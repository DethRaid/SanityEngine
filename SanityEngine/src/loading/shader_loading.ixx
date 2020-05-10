module;

#include <string>
#include <vector>
#include <minitrace.h>
#include <spdlog/spdlog.h>

export module shader_loading;

export std::vector<uint8_t> load_shader(const std::string& shader_filename){
    MTR_SCOPE("Loading", "load_shader");
    auto* shader_file = fopen(shader_filename.c_str(), "rb");
    if(shader_file == nullptr) {
        spdlog::error("Could not open shader file '{}'", shader_filename);
        return {};
    }

    fseek(shader_file, 0, SEEK_END);
    const auto file_size = ftell(shader_file);

    auto shader = std::vector<uint8_t>(file_size);

    rewind(shader_file);

    fread(shader.data(), sizeof(uint8_t), file_size, shader_file);
    fclose(shader_file);

    return shader;
}

