#pragma once

#include <random>

#include <d3d12.h>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <spdlog/logger.h>

#include <rx/core/vector.h>

namespace renderer {
    class HostTexture2D {
    public:
        template <typename RandomDeviceType>
        static HostTexture2D create_random(const glm::uvec2& size, RandomDeviceType& rng);

        /*!
         * \brief Instantiates a new 2D texture
         *
         * \param size_in Size, in pixels, of the texture
         * \param texels_in Pixels of the texture, in row-major order
         */
        HostTexture2D(const glm::uvec2& size_in, Rx::Vector<glm::u8vec4> texels_in);

        [[nodiscard]] glm::u8vec4 sample_linear(const D3D12_SAMPLER_DESC& sampler_desc, const glm::vec2& texcoord) const;

        [[nodiscard]] glm::u8vec4 sample_point(const D3D12_SAMPLER_DESC& sampler_desc, const glm::vec2& texcoord) const;

        [[nodiscard]] glm::u8vec4 sample(const D3D12_SAMPLER_DESC& sampler_desc, const glm::vec2& uv) const;

        [[nodiscard]] glm::uvec2 get_size() const;

    private:
        static std::shared_ptr<spdlog::logger> logger;

        glm::vec2 size;

        glm::vec2 texel_size;

        Rx::Vector<glm::u8vec4> texels;
    };

    template <typename RandomDeviceType>
    HostTexture2D HostTexture2D::create_random(const glm::uvec2& size, RandomDeviceType& rng) {
        Rx::Vector<uint32_t> texels(size.x * size.y);

        for(auto& texel : texels) {
            texel = rng();
        }

        const auto* typed_texels = reinterpret_cast<glm::u8vec4*>(texels.data());

        return {size, Rx::Vector{texels.disown()}};
    }
} // namespace renderer
