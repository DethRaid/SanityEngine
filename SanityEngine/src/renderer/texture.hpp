#pragma once

#include <vector>

#include <d3d12.h>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace renderer {
    class Texture2D {
    public:
        /*!
         * \brief Instantiates a new 2D texture
         *
         * \param size Size, in pixels, of the texture
         * \param pixels_in Pixels of the texture, in row-major order
         */
        Texture2D(const glm::uvec2& size, const std::vector<glm::u8vec4>& pixels_in);

        [[nodiscard]] glm::u8vec4 sample(const D3D12_SAMPLER_DESC& sampler_desc, const glm::vec2& uv) const;

        [[nodiscard]] glm::uvec2 get_size() const;
    };
} // namespace renderer
 