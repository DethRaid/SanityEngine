#include "render_components.hpp"

#include <imgui/imgui.h>

#include "ui/property_drawers.hpp"

namespace sanity::engine::renderer {
    void draw_component_editor(StandardRenderableComponent& renderable) {
        ui::draw_property_editor("Mesh", renderable.mesh);
        ui::draw_property_editor("Material", renderable.material);
        ui::draw_property_editor("Is background", renderable.is_background);
    }

    void draw_component_editor(PostProcessingPassComponent& post_processing) {
        ui::draw_property_editor("Draw Index", post_processing.draw_idx);
        ui::draw_property_editor("Material", post_processing.material);
    }

    void draw_component_editor(RaytracingObjectComponent& raytracing_object) {
        ImGui::LabelText("Handle", "%#010x", raytracing_object.as_handle.index);
    }

    void draw_component_editor(CameraComponent& camera) {
        ui::draw_property_editor("FOV", camera.fov);
        ui::draw_property_editor("Aspect Ratio", camera.aspect_ratio);
        ui::draw_property_editor("Near clip plane", camera.near_clip_plane);

        if(camera.fov <= 0) {
            ui::draw_property_editor("Orthographic size", camera.orthographic_size);
            camera.fov = 0;
        }
    }

    void draw_component_editor(LightComponent& light) {
        ui::draw_property_editor("Type", light.type);
        ui::draw_property_editor("Color", light.color);
        ui::draw_property_editor("Size", light.size);
    }

    void draw_component_editor(SkyComponent& sky) {
        // Fill in when the component has controls to draw
    }
} // namespace sanity::engine::renderer
