#pragma once

#include <memory>

#include <bve/bve.hpp>
#include <d3d11.h>
#include <entt/entity/fwd.hpp>
#include <rx/core/function.h>
#include <rx/core/vector.h>

#include "core/async/synchronized_resource.hpp"
#include "rhi/bind_group.hpp"
#include "rhi/compute_pipeline_state.hpp"
#include "rhi/mesh_data_store.hpp"

namespace renderer {
    class Renderer;
}

// Ths one thing that's allowed to be a `std::unique_ptr`. We need want the custom deleter. This is kinda an anonymous class, except C++ has
// bad support for those
using BveMeshHandle = std::unique_ptr<bve::BVE_Loaded_Static_Mesh, Rx::Function<void(bve::BVE_Loaded_Static_Mesh*)>>;

class BveWrapper {
public:
    explicit BveWrapper(ID3D11Device* device);

    [[nodiscard]] bool add_train_to_scene(const Rx::String& filename,
                                          SynchronizedResource<entt::registry>& registry,
                                          renderer::Renderer& renderer);

private:
    ComPtr<ID3D11ComputeShader> bve_texture_pipeline;

    void create_texture_filter_pipeline(ID3D11Device* device);

    [[nodiscard]] bve::BVE_User_Error_Data get_printable_error(const bve::BVE_Mesh_Error& error);

    [[nodiscard]] BveMeshHandle load_mesh_from_file(const Rx::String& filename);

    [[nodiscard]] Rx::Pair<Rx::Vector<StandardVertex>, Rx::Vector<Uint32>> process_vertices(const bve::BVE_Mesh& mesh) const;
};
