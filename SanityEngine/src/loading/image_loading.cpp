#include "image_loading.hpp"

#include "Tracy.hpp"
#include "TracyD3D12.hpp"
#include "adapters/rex/rex_wrapper.hpp"
#include "renderer/renderer.hpp"
#include "renderer/rhi/d3d12_private_data.hpp"
#include "renderer/rhi/helpers.hpp"
#include "renderer/rhi/render_device.hpp"
#include "renderer/rhi/resources.hpp"
#include "rx/core/log.h"
#include "sanity_engine.hpp"
#include "stb_image.h"

namespace sanity::engine {
    RX_LOG("TextureLoading", logger);

    template <typename ComponentDataType>
    ComponentDataType* copy_and_pad_texture_data(ComponentDataType* original_data,
                                               Uint32 width,
                                               Uint32 height,
                                               Uint32 original_num_components);

    void* load_texture(const std::filesystem::path& texture_name, Uint32& width, Uint32& height, renderer::TextureFormat& format) {
        ZoneScoped;

        const auto& exe_directory = SanityEngine::executable_directory;
        const auto full_texture_path = exe_directory / texture_name;

        int raw_width, raw_height, num_components;

        void* texture_data{nullptr};

        const auto full_texture_path_string = full_texture_path.string();
        if(stbi_is_hdr(full_texture_path_string.c_str())) {
            logger->verbose("Loading image %s as RGBA32f HDR", texture_name);
            auto* data = stbi_loadf(full_texture_path_string.c_str(), &raw_width, &raw_height, &num_components, 0);
            if(data != nullptr) {
                format = renderer::TextureFormat::Rgba32F;
                texture_data = copy_and_pad_texture_data(data, raw_width, raw_height, num_components);
                stbi_image_free(data);
            }

        } else {
            logger->verbose("Loading image %s as RGBA8 LDR", texture_name);
            auto* data = stbi_load(full_texture_path_string.c_str(), &raw_width, &raw_height, &num_components, 0);
            if(data != nullptr) {
                format = renderer::TextureFormat::Rgba8;
                texture_data = copy_and_pad_texture_data(data, raw_width, raw_height, num_components);
                stbi_image_free(data);
            }
        }

        if(texture_data == nullptr) {
            const auto* failure_reason = stbi_failure_reason();
            logger->error("Could not load image %s: %s", texture_name, failure_reason);
            return nullptr;
        }

        width = static_cast<Uint32>(raw_width);
        height = static_cast<Uint32>(raw_height);

        return texture_data;
    }

    Rx::Optional<renderer::TextureHandle> load_texture_to_gpu(const std::filesystem::path& texture_name, renderer::Renderer& renderer) {
        ZoneScoped;

        Uint32 width, height;
        renderer::TextureFormat format;
        const auto* pixels = load_texture(texture_name, width, height, format);
        if(pixels == nullptr) {
            return Rx::nullopt;
        }

        const auto texture_name_string = texture_name.string();
        const auto create_info = renderer::TextureCreateInfo{.name = texture_name_string.c_str(),
                                                             .usage = renderer::TextureUsage::SampledTexture,
                                                             .format = format,
                                                             .width = width,
                                                             .height = height};

        auto& device = renderer.get_render_backend();
        auto commands = device.create_command_list();

        const auto msg = Rx::String::format("load_texture_to_gpu(%s)", texture_name);
        renderer::set_object_name(commands.Get(), msg);

        renderer::TextureHandle handle_out{0};

        {
            TracyD3D12Zone(renderer::RenderBackend::tracy_context, commands.Get(), msg.data());
            PIXScopedEvent(commands.Get(), PIX_COLOR_DEFAULT, msg.data());

            handle_out = renderer.create_texture(create_info, pixels, commands.Get());
        }

        device.submit_command_list(Rx::Utility::move(commands));

        return handle_out;
    }

    constexpr const Uint64 DESIRED_NUM_COMPONENTS = 4;

    template <typename ComponentDataType>
    ComponentDataType* copy_and_pad_texture_data(ComponentDataType* original_data,
                                               const Uint32 width,
                                               const Uint32 height,
                                               const Uint32 original_num_components) {
        ZoneScoped;

        const auto num_pixels = static_cast<Size>(width) * height;

        auto* pixels = new ComponentDataType[num_pixels * DESIRED_NUM_COMPONENTS];

        if(original_num_components == DESIRED_NUM_COMPONENTS) {
            memcpy(pixels, original_data, num_pixels * DESIRED_NUM_COMPONENTS * sizeof(ComponentDataType));

        } else {
            for(Uint32 i = 0; i < num_pixels; i++) {
                const auto read_idx = i * original_num_components;
                const auto write_idx = i * DESIRED_NUM_COMPONENTS;

                pixels[write_idx] = original_data[read_idx];
                pixels[write_idx + 1] = original_data[read_idx + 1];
                pixels[write_idx + 2] = original_data[read_idx + 2];

                if(original_num_components == DESIRED_NUM_COMPONENTS) {
                    pixels[write_idx + 3] = original_data[read_idx + 3];

                } else {
                    pixels[write_idx + 3] = 0xFF;
                }
            }
        }

        return pixels;
    }

} // namespace sanity::engine
