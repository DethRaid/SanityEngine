#include "image_loading.hpp"

#include <stb_image.h>
#include <spdlog/spdlog.h>

bool load_image(const std::string& image_name, uint32_t& width, uint32_t& height, std::vector<uint8_t>& pixels) {
    int raw_width, raw_height, num_components;

    const auto* texture_data = stbi_load(image_name.c_str(), &raw_width, &raw_height, &num_components, 0);
    if(texture_data == nullptr) {
        const auto* failure_reason = stbi_failure_reason();
        spdlog::error("Could not load image {}: {}", image_name, failure_reason);
        return false;
    }

    width = static_cast<uint32_t>(raw_width);
    height = static_cast<uint32_t>(raw_height);

    const auto num_pixels = width * height;
    pixels.resize(num_pixels * 4);
    for(uint32_t i = 0; i < num_pixels; i++) {
        const auto read_idx = i * num_components;
        const auto write_idx = i * 4;

        pixels[write_idx] = texture_data[read_idx];
        pixels[write_idx + 1] = texture_data[read_idx + 1];
        pixels[write_idx + 2] = texture_data[read_idx + 2];

        if(num_components == 4) {
            pixels[write_idx + 3] = texture_data[read_idx + 3];

        } else {
            pixels[write_idx + 4] = 0xFF;
        }
    }

    return true;
}
