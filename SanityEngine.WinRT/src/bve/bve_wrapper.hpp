#pragma once

#include <functional>
#include <memory>

#include "bve/bve.hpp"
#include "core/async/synchronized_resource.hpp"
#include "entt/entity/fwd.hpp"
#include "renderer/rhi/bind_group.hpp"
#include "renderer/rhi/mesh_data_store.hpp"
#include "rx/core/ptr.h"
#include "rx/core/vector.h"

namespace renderer {
    class Renderer;
}

// Ths one thing that's allowed to be a `std::unique_ptr`. We need want the custom deleter. This is kinda an anonymous class, except C++ has
// bad support for those
using BveMeshHandle = std::unique_ptr<bve::BVE_Loaded_Static_Mesh, std::function<void(bve::BVE_Loaded_Static_Mesh*)>>;

class BveWrapper {
public:
    explicit BveWrapper(renderer::RenderBackend& device);

    [[nodiscard]] bool add_train_to_scene(const Rx::String& filename,
                                          SynchronizedResource<entt::registry>& registry,
                                          renderer::Renderer& renderer);
    Rx::Ptr<renderer::BindGroupBuilder> create_texture_processor_bind_group_builder(renderer::RenderBackend& device);

private:
    com_ptr<ID3D12PipelineState> bve_texture_pipeline;

    void create_texture_filter_pipeline(renderer::RenderBackend& device);

    [[nodiscard]] bve::BVE_User_Error_Data get_printable_error(const bve::BVE_Mesh_Error& error);

    [[nodiscard]] BveMeshHandle load_mesh_from_file(const Rx::String& filename);

    [[nodiscard]] std::pair<Rx::Vector<StandardVertex>, Rx::Vector<Uint32>> process_vertices(const bve::BVE_Mesh& mesh) const;
};
