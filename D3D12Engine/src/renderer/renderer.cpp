#include "renderer.hpp"

#include <entt/entt.hpp>

#include "../core/components.hpp"
#include "../rhi/render_device.hpp"
#include "components.hpp"

namespace renderer {
    constexpr uint32_t STATIC_MESH_VERTEX_BUFFER_SIZE = 64 << 20;
    constexpr uint32_t STATIC_MESH_INDEX_BUFFER_SIZE = 64 << 20;

    Renderer::Renderer(GLFWwindow* window) : render_device{make_render_device(rhi::RenderBackend::D3D12, window)} {
        make_static_mesh_storage();
    }

    void Renderer::render_scene(entt::registry& registry) {
        auto command_list = render_device->create_render_command_list();

        render_3d_scene(registry, *command_list);

        render_device->submit_command_list(std::move(command_list));
    }

    StaticMeshRenderable Renderer::create_static_mesh(const std::vector<rhi::BveVertex>& vertices, const std::vector<uint32_t>& indices) const {
        const auto mesh_start_idx = static_mesh_storage->add_mesh(vertices, indices);

        StaticMeshRenderable renderable {};
        renderable.first_index = mesh_start_idx;
        renderable.num_indices = indices.size();

        return renderable;
    }

    void Renderer::make_static_mesh_storage() {
        rhi::BufferCreateInfo vertex_create_info {};
        vertex_create_info.name = "Static Mesh Vertex Buffer";
        vertex_create_info.size = STATIC_MESH_VERTEX_BUFFER_SIZE;
        vertex_create_info.usage = rhi::BufferUsage::VertexBuffer;

        auto vertex_buffer = render_device->create_buffer(vertex_create_info);

        rhi::BufferCreateInfo index_buffer_create_info {};
        index_buffer_create_info.name = "Static Mesh Index Buffer";
        index_buffer_create_info.size = STATIC_MESH_INDEX_BUFFER_SIZE;
        index_buffer_create_info.usage = rhi::BufferUsage::IndexBuffer;

        auto index_buffer = render_device->create_buffer(index_buffer_create_info);

        static_mesh_storage = std::make_unique<rhi::MeshDataStore>(*render_device, std::move(vertex_buffer), std::move(index_buffer));
    }

    void Renderer::render_3d_scene(entt::registry& registry, rhi::RenderCommandList& command_list) {
        command_list.bind_mesh_data(*static_mesh_storage);

        auto static_mesh_view = registry.view<Transform, StaticMeshRenderable>();

        for(auto entity : static_mesh_view) {
            const auto transform = registry.get<Transform>(entity);
            const auto mesh_renderable = registry.get<StaticMeshRenderable>(entity);

            command_list.draw(mesh_renderable.num_indices, mesh_renderable.first_index);
        }
    }
} // namespace renderer
