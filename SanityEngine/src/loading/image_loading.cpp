#include "image_loading.hpp"

#include <minitrace.h>
#include <spdlog/spdlog.h>
#include <stb_image.h>

#include "../renderer/renderer.hpp"
#include "../rhi/render_device.hpp"
#include "../rhi/resources.hpp"

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
            pixels[write_idx + 3] = 0xFF;
        }
    }

    return true;
}

void load_image_to_gpu(ftl::TaskScheduler* /* scheduler */, void* data) {
    auto* load_data = static_cast<LoadImageToGpuData*>(data);

    const auto message = fmt::format("Load image {}", load_data->texture_name);
    MTR_SCOPE("Image Loading", message.c_str());

    uint32_t width, height;
    std::vector<uint8_t> pixels;
    const auto success = load_image(load_data->texture_name, width, height, pixels);
    if(!success) {
        load_data->handle = std::nullopt;
        return;
    }

    const auto create_info = rhi::ImageCreateInfo{.name = load_data->texture_name,
                                                  .usage = rhi::ImageUsage::SampledImage,
                                                  .format = rhi::ImageFormat::Rgba8,
                                                  .width = width,
                                                  .height = height};

    auto& device = load_data->renderer->get_render_device();
    const auto commands = device.create_command_list();
    load_data->handle = load_data->renderer->create_image(create_info, pixels.data(), commands);
    device.submit_command_list(commands);
}
