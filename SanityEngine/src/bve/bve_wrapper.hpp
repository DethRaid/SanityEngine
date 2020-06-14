#pragma once

#include <functional>
#include <memory>

#include <bve/bve.hpp>
#include <entt/entity/fwd.hpp>
#include <rx/core/vector.h>

#include "rhi/bind_group.hpp"
#include "rhi/compute_pipeline_state.hpp"
#include "rhi/mesh_data_store.hpp"
#include <rx/core/ptr.h>

namespace renderer {
    class Renderer;
}

using BveMeshHandle = std::unique_ptr<bve::BVE_Loaded_Static_Mesh, std::function<void(bve::BVE_Loaded_Static_Mesh*)>>;

class BveWrapper {
public:
    explicit BveWrapper(rhi::RenderDevice& device);

    [[nodiscard]] bool add_train_to_scene(const Rx::String& filename, entt::registry& registry, renderer::Renderer& renderer);
    Rx::Ptr<rhi::BindGroupBuilder> create_texture_processor_bind_group_builder(rhi::RenderDevice& device);

private:
    Rx::Ptr<rhi::ComputePipelineState> bve_texture_pipeline;

    void create_texture_filter_pipeline(rhi::RenderDevice& device);

    [[nodiscard]] bve::BVE_User_Error_Data get_printable_error(const bve::BVE_Mesh_Error& error);

    [[nodiscard]] BveMeshHandle load_mesh_from_file(const Rx::String& filename);

    [[nodiscard]] std::pair<Rx::Vector<StandardVertex>, Rx::Vector<uint32_t>> process_vertices(const bve::BVE_Mesh& mesh) const;
};
