#include "world_generation.hpp"

#include <cstdint>

#include <glm/vec2.hpp>

#include "../../renderer/texture.hpp"

constexpr uint32_t NUM_OCTAVES = 5;

float get_terrain_height(const TerrainSamplerParams& params, const renderer::Texture2D& noise_texture) {
    const static D3D12_SAMPLER_DESC NOISE_SAMPLER{.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
                                                  .AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                                  .AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP};
    // Generate terrain whooooooo

    // General idea:
    //
    // Sample the noise texture for a lot of octaves of noise
    // Octave 0 has a resolution of eight texels from the north pole to the south pole, and sixteen texels from the westernmost edge of the
    // world to the easternmost edge
    //
    // Octave 1 has twice the resolution of octave 0
    //
    // Octave 2 has twice the resolution of octave 1
    //
    // etc

    const glm::vec2 octave_0_scale = noise_texture.get_size() / 4u;
    glm::vec2 texcoord = glm::vec2{params.longitude / (TERRAIN_LONGITUDE_RANGE * 2), params.latitude / (TERRAIN_LATITUDE_RANGE * 2)} *
                         octave_0_scale;

    double terrain_height = 0;
    float spread = params.spread;

    for(uint32_t i = 0; i < NUM_OCTAVES; i++) {
        const auto& noise_sample = noise_texture.sample(NOISE_SAMPLER, texcoord);
        const auto height_sample = static_cast<double>(noise_sample.x) / 255.0;
        terrain_height += height_sample * spread;

        spread *= params.spread_reduction_rate;
        texcoord *= 2;
    }

    const auto terrain_height_range = MAX_TERRAIN_HEIGHT - MIN_TERRAIN_HEIGHT;
    return static_cast<float>(terrain_height * terrain_height_range + MIN_TERRAIN_HEIGHT);
}
