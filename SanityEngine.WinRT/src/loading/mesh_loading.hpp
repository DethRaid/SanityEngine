#pragma once

#include <d3d12.h>
#include <winrt/base.h>

#include "renderer/mesh.hpp"
#include "rhi/mesh_data_store.hpp"
#include "rx/core/optional.h"

namespace renderer {
    class Renderer;
}

namespace Rx {
    struct String;
}

using winrt::com_ptr;

/*!
 * \brief Loads a mesh from disk
 *
 * \param filepath The absolute filepath to the mesh
 * \param commands The command list to use to upload mesh data
 * \param renderer The renderer that will eventually render the mesh
 */
Rx::Optional<renderer::MeshObject> import_mesh(const Rx::String& filepath,
                                               com_ptr<ID3D12GraphicsCommandList4> commands,
                                               renderer::Renderer& renderer);
