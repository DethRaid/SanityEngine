#include "textures.hpp"

#include <utility>

#include <glm/glm.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace renderer {
    std::shared_ptr<spdlog::logger> HostTexture2D::logger = spdlog::stdout_color_st("Texture2D");

    HostTexture2D::HostTexture2D(const glm::uvec2& size_in, std::vector<glm::u8vec4> texels_in)
        : size{size_in}, texel_size{1.0f / size}, texels{std::move(texels_in)} {}

    glm::u8vec4 HostTexture2D::sample_linear(const D3D12_SAMPLER_DESC& sampler_desc, const glm::vec2& texcoord) const {
        const auto texcoord_in_texels = texcoord * size;

        const auto texcoord_0_0 = floor(texcoord_in_texels);
        const auto texcoord_1_1 = texcoord + 1.0f;
        const auto texcoord_1_0 = glm::vec2{texcoord_1_1.x, texcoord_0_0.y};
        const auto texcoord_0_1 = glm::vec2{texcoord_0_0.x, texcoord_1_1.y};

        const auto texel_0_0 = sample_point(sampler_desc, texcoord_0_0 / size);
        const auto texel_0_1 = sample_point(sampler_desc, texcoord_0_1 / size);
        const auto texel_1_0 = sample_point(sampler_desc, texcoord_1_0 / size);
        const auto texel_1_1 = sample_point(sampler_desc, texcoord_1_1 / size);

        const auto interpolation_amt = glm::fract(texcoord_in_texels);

        const auto top_texel = mix(glm::vec4{texel_0_1}, glm::vec4{texel_0_0}, interpolation_amt.x);
        const auto bottom_texel = mix(glm::vec4{texel_1_1}, glm::vec4{texel_1_0}, interpolation_amt.x);
        const auto actual_texel = mix(bottom_texel, top_texel, interpolation_amt.y);

        return glm::u8vec4{actual_texel};
    }

    glm::u8vec4 HostTexture2D::sample_point(const D3D12_SAMPLER_DESC& sampler_desc, const glm::vec2& texcoord) const {
        glm::uvec2 texel_coords;

        switch(sampler_desc.AddressU) {
            case D3D12_TEXTURE_ADDRESS_MODE_WRAP:
                texel_coords.x = static_cast<uint32_t>(glm::fract(texcoord.x) * size.x);
                break;

            case D3D12_TEXTURE_ADDRESS_MODE_CLAMP:
                if(texcoord.x < 0) {
                    texel_coords.x = 0;
                } else if(texcoord.x > 1) {
                    texel_coords.x = size.x;
                } else {
                    texel_coords.x = static_cast<uint32_t>(texcoord.x * size.x);
                }
                break;

            default:
                logger->error("Unsupported texture address mode {} - defaulting to sampling pixel 0", sampler_desc.AddressU);
                texel_coords.x = 0;
        }

        switch(sampler_desc.AddressV) {
            case D3D12_TEXTURE_ADDRESS_MODE_WRAP:
                texel_coords.y = static_cast<uint32_t>(glm::fract(texcoord.y) * size.y);
                break;

            case D3D12_TEXTURE_ADDRESS_MODE_CLAMP:
                if(texcoord.y < 0) {
                    texel_coords.y = 0;
                } else if(texcoord.y > 1) {
                    texel_coords.y = static_cast<uint32_t>(size.y);
                } else {
                    texel_coords.y = static_cast<uint32_t>(round(texcoord.y * size.y));
                }
                break;

            default:
                logger->error("Unsupported texture address mode {} - defaulting to sampling pixel 0", sampler_desc.AddressV);
                texel_coords.y = 0;
        }

        const auto texel_index = static_cast<uint32_t>(round(texel_coords.y * size.y) + texel_coords.x);

        return texels[texel_index];
    }

    glm::u8vec4 HostTexture2D::sample(const D3D12_SAMPLER_DESC& sampler_desc, const glm::vec2& uv) const {
        if(sampler_desc.Filter == D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR) {
            return sample_linear(sampler_desc, uv);

        } else if(sampler_desc.Filter == D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT) {
            return sample_point(sampler_desc, uv);
        }

        logger->error("Unsupported sampler type {}", sampler_desc.Filter);

        return {};
    }

    glm::uvec2 HostTexture2D::get_size() const { return size; }
} // namespace renderer
