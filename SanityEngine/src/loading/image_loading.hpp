#pragma once

#include <filesystem>

#include "renderer/handles.hpp"
#include "renderer/rhi/resources.hpp"
#include "rx/core/optional.h"

namespace sanity::engine {
    namespace renderer {
        class Renderer;
    }

    void* load_texture(const std::filesystem::path& texture_name, Uint32& width, Uint32& height, renderer::TextureFormat& format);

    Rx::Optional<renderer::TextureHandle> load_texture_to_gpu(const std::filesystem::path& texture_name, renderer::Renderer& renderer);
} // namespace sanity::engine
