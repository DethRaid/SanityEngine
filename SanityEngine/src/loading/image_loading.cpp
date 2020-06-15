#include "image_loading.hpp"

#include <Tracy.hpp>
#include <ftl/task.h>
#include <minitrace.h>
#include <rx/core/log.h>
#include <stb_image.h>

#include "renderer/renderer.hpp"
#include "rhi/render_device.hpp"
#include "rhi/resources.hpp"

RX_LOG("ImageLoading", logger);

bool load_image(const Rx::String& image_name, Uint32& width, Uint32& height, Rx::Vector<uint8_t>& pixels) {
    int raw_width, raw_height, num_components;

    const auto* texture_data = stbi_load(image_name.data(), &raw_width, &raw_height, &num_components, 0);
    if(texture_data == nullptr) {
        const auto* failure_reason = stbi_failure_reason();
        logger->error("Could not load image %s: %s", image_name, failure_reason);
        return false;
    }

    width = static_cast<Uint32>(raw_width);
    height = static_cast<Uint32>(raw_height);

    const auto num_pixels = width * height;
    pixels.resize(num_pixels * 4);
    for(Uint32 i = 0; i < num_pixels; i++) {
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

FTL_TASK_ENTRY_POINT(load_image_to_gpu) {
    auto* load_data = static_cast<LoadImageToGpuArgs*>(arg);

    const auto message = Rx::String::format("Load image %s", load_data->texture_name_in);
    ZoneScopedN(message.data());

    Uint32 width, height;
    Rx::Vector<uint8_t> pixels;
    const auto success = load_image(load_data->texture_name_in, width, height, pixels);
    if(!success) {
        load_data->handle_out = Rx::nullopt;
        return;
    }

    const auto create_info = rhi::ImageCreateInfo{.name = load_data->texture_name_in,
                                                  .usage = rhi::ImageUsage::SampledImage,
                                                  .format = rhi::ImageFormat::Rgba8,
                                                  .width = width,
                                                  .height = height};

    auto& device = load_data->renderer_in->get_render_device();
    const auto commands = device.create_command_list();
    load_data->handle_out = load_data->renderer_in->create_image(create_info, pixels.data(), commands);
    device.submit_command_list(commands);
}
