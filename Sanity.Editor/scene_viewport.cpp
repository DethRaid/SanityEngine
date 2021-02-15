#include "scene_viewport.hpp"

#include "renderer/rhi/d3d12_private_data.hpp"

#include "Tracy.hpp"
#include "renderer/handles.hpp"
#include "renderer/renderer.hpp"

using namespace sanity::engine::renderer;

namespace sanity::editor::ui {
    RX_LOG("SceneViewport", logger);

    SceneViewport::SceneViewport(Renderer& renderer_in) : Window{"Scene Viewport"}, renderer{&renderer_in} {}

    void SceneViewport::set_render_size(const Uint2& new_render_size) {
        if(new_render_size == render_size) {
            return;
        }

        render_size = new_render_size;

        recreate_scene_output_texture();
    }

    void SceneViewport::draw_contents() {
        const auto content_region_min = ImGui::GetWindowContentRegionMin();
        const auto content_region_max = ImGui::GetWindowContentRegionMax();
        const auto im_size = ImVec2{content_region_max.x - content_region_min.x, content_region_max.y - content_region_min.y};

        ImGui::Image(scene_output_texture, im_size);

        if(ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            logger->info("User clicked on scene output texture");
        }

        const auto size = Uint2{static_cast<Uint32>(im_size.x), static_cast<Uint32>(im_size.y)};
        if(size != render_size) {
            set_render_size(size);
        }
    }

    void SceneViewport::recreate_scene_output_texture() {
        ZoneScoped;

        if(scene_output_texture != nullptr) {
            const auto scene_output_texture_handle_u64 = reinterpret_cast<Uint64>(scene_output_texture);
            const auto scene_output_texture_handle_index = static_cast<Uint32>(scene_output_texture_handle_u64);
            const auto scene_output_texture_handle = TextureHandle{scene_output_texture_handle_index};

            renderer->schedule_texture_destruction(scene_output_texture_handle);

            scene_output_texture = nullptr;
        }

        const auto create_info = TextureCreateInfo{.name = "Scene viewport texture",
                                                   .usage = TextureUsage::RenderTarget,
                                                   .format = TextureFormat::Rgba8,
                                                   .width = render_size.x,
                                                   .height = render_size.y};

        const auto scene_output_texture_handle = renderer->create_texture(create_info);
        const auto scene_output_texture_index = static_cast<Uint64>(scene_output_texture_handle.index);
        scene_output_texture = reinterpret_cast<void*>(scene_output_texture_index);

        renderer->set_scene_output_texture(scene_output_texture_handle);
    }
} // namespace sanity::editor::ui
