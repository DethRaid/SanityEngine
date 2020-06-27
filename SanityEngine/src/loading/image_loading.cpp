#include "image_loading.hpp"

#include <Tracy.hpp>
#include <TracyD3D12.hpp>
#include <ftl/task.h>
#include <rx/core/log.h>
#include <stb_image.h>

#include "renderer/renderer.hpp"
#include "rhi/helpers.hpp"
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

Rx::Optional<renderer::TextureHandle> load_image_to_gpu(const Rx::String& texture_name, renderer::Renderer& renderer) {
    ZoneScoped;

    Uint32 width, height;
    Rx::Vector<uint8_t> pixels;
    const auto success = load_image(texture_name, width, height, pixels);
    if(!success) {
        return Rx::nullopt;
    }

    const auto create_info = renderer::ImageCreateInfo{.name = texture_name,
                                                       .usage = renderer::ImageUsage::SampledImage,
                                                       .format = renderer::ImageFormat::Rgba8,
                                                       .width = width,
                                                       .height = height};

    auto& device = renderer.get_render_device();
    auto commands = device.create_command_list();

    const auto msg = Rx::String::format("load_image_to_gpu(%s)", texture_name);
    renderer::set_object_name(commands.cmds, msg);

    renderer::TextureHandle handle_out;

    {
        TracyD3D12Zone(renderer::RenderDevice::tracy_context, commands.Get(), msg.data());
        PIXScopedEvent(commands.cmds.Get(), PIX_COLOR_DEFAULT, msg.data());

        // I get this error from PIXScopedEvent
        //
        // D3D12 ERROR: ID3D12GraphicsCommandList::*: This API cannot be called on a closed command list. [ EXECUTION ERROR #547:
        // COMMAND_LIST_CLOSED]
        //
        // This means... the device is giving me a closed command list?
        //
        // I'm creating a new command allocator for each command list, and I'm creating a new command list every time I call
        // RenderDevice::create_command_list. Does my command list come pre-closed? Do I have to begin my command list or something?

        handle_out = renderer.create_image(create_info, pixels.data(), commands.cmds);
    }

    device.submit_command_list(Rx::Utility::move(commands));

    return handle_out;
}
