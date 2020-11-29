#pragma once

#include <d3d12.h>

#include "core/types.hpp"

struct TerrainData;

namespace renderer {
    class RenderBackend;
    class Renderer;
} // namespace renderer

namespace sanity::engine::terraingen {
    /*!
     * \brief Creates the PSOs that will be used for terrain generation
     */
    void initialize(renderer::RenderBackend& device);

    /*!
     * \brief Places oceans in the provided heightmap
     *
     * \param commands The command list to record ocean-placing commands in to
     * \param renderer The renderer which will render this terrain
     * \param sea_level Height of the sea, on average
     * \param data The TerrainData that will hold the ocean depth texture
     */
    void place_oceans(const ComPtr<ID3D12GraphicsCommandList4>& commands,
                      renderer::Renderer& renderer,
                      Uint32 sea_level,
                      TerrainData& data);

    /*!
     * \brief Does some useful version of flood-filling water
     *
     * \param commands The command list to perform the water filling on
     * \param renderer The renderer that holds the terrain images
     * \param data The terrain data to compute water flow on
     */
    void compute_water_flow(const ComPtr<ID3D12GraphicsCommandList4>& commands, renderer::Renderer& renderer, TerrainData& data);
} // namespace terraingen
