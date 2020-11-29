#pragma once

#include <d3d12.h>

#include "renderer/mesh.hpp"
#include "renderer/rhi/mesh_data_store.hpp"
#include "rx/core/optional.h"

namespace Rx {
    struct String;
}

namespace sanity::engine {
    namespace renderer {
        class Renderer;
    }

    /*!
     * \brief Loads a mesh from disk
     *
     * \param filepath The absolute filepath to the mesh
     * \param commands The command list to use to upload mesh data
     * \param renderer The renderer that will eventually render the mesh
     */
    [[nodiscard]] ::Rx::Optional<renderer::MeshObject> import_mesh(const Rx::String& filepath,
                                                                 ComPtr<ID3D12GraphicsCommandList4> commands,
                                                                 renderer::Renderer& renderer);
} // namespace sanity::engine
