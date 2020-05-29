#pragma once

#include <vector>

#include <d3d12.h>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <spdlog/logger.h>

namespace renderer {
    class Texture2D {
    public:
        /*!
         * \brief Instantiates a new 2D texture
         *
         * \param size_in Size, in pixels, of the texture
         * \param texels_in Pixels of the texture, in row-major order
         */
        Texture2D(const glm::uvec2& size_in, std::vector<glm::u8vec4> texels_in);

        [[nodiscard]] glm::u8vec4 sample_linear(const D3D12_SAMPLER_DESC& sampler_desc, const glm::vec2& texcoord) const;

        [[nodiscard]] glm::u8vec4 sample_point(const D3D12_SAMPLER_DESC& sampler_desc, const glm::vec2& texcoord) const;

        [[nodiscard]] glm::u8vec4 sample(const D3D12_SAMPLER_DESC& sampler_desc, const glm::vec2& uv) const;

        [[nodiscard]] glm::uvec2 get_size() const;

    private:
        static std::shared_ptr<spdlog::logger> logger;

        glm::vec2 size;

        glm::vec2 texel_size;

        std::vector<glm::u8vec4> texels;
    };
} // namespace renderer
