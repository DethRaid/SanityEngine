#include "resources.hpp"

#include "renderer/renderer.hpp"
#include "rx/core/log.h"

namespace sanity::engine::renderer {

    RX_LOG("PerFrameBuffer", logger);

    BufferRing::BufferRing(const Rx::String& name, Uint32 size, Renderer& renderer) {
        const auto num_frames = renderer.get_render_backend().get_max_num_gpu_frames();
        resources.reserve(num_frames);

        for(auto i = 0u; i < num_frames; i++) {
            const auto create_info = BufferCreateInfo{.name = Rx::String::format("%s buffer %d", name, i),
                                                      .usage = BufferUsage::ConstantBuffer,
                                                      .size = size};
            const auto handle = renderer.create_buffer(create_info);

            resources.push_back(handle);
        }
    }

    void TextureRing::add_texture(const TextureHandle texture) { resources.push_back(texture); }

    void FluidVolumeResourceRing::add_buffer_pair(const TextureHandle buffer0, const TextureHandle buffer1) {
        resources.emplace_back(buffer0, buffer1);
    }

    glm::uvec3 FluidVolume::get_voxel_size() const {
        const auto power = glm::ceil(glm::log2(glm::vec3{size} * voxels_per_meter));
        return glm::uvec3{glm::pow(glm::vec3{2.f}, power)};
    }

    Uint32 size_in_bytes(const TextureFormat format) {
        switch(format) {
            case TextureFormat::Rgba8:
                [[fallthrough]];
            case TextureFormat::Rg16F:
                [[fallthrough]];
            case TextureFormat::R32F:
                [[fallthrough]];
            case TextureFormat::R32UInt:
                [[fallthrough]];
            case TextureFormat::Depth32:
                [[fallthrough]];
            case TextureFormat::Depth24Stencil8:
                return 4;

            case TextureFormat::Rg32F:
                [[fallthrough]];
            case TextureFormat::Rgba16F:
                return 8;

            case TextureFormat::Rgba32F:
                return 16;

            default:
                return 4;
        }
    }
} // namespace sanity::engine::renderer
