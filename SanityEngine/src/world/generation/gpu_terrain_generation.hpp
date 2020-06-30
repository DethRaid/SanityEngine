#pragma once

#include <d3d12.h>
#include <wrl/client.h>

namespace renderer {
    class RenderDevice;
}

using Microsoft::WRL::ComPtr;

namespace terraingen {
    /*!
     * \brief Creates the PSOs that will be used for terrain generation
     */
    void create_pipelines(renderer::RenderDevice& device);

    /*!
     * \brief Places oceans in the provided heightmap
     *
     * \param cmds The command list to record ocean-placing commands in to
     * \param height_water_map A RGBA32F texture with terrain height in the red channel, and nothing meaningful in the other channels. After
     * we place oceans, the texture with have the terrain height in its red channel and water depth in its green
     * \param sea_level Height of the sea, on average
     */
    void place_oceans(ComPtr<ID3D12GraphicsCommandList4> cmds, ComPtr<ID3D12Resource> height_water_map, float sea_level);
} // namespace terraingen
