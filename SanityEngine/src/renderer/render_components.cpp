#include "render_components.hpp"

#include <imgui/imgui.h>

#include "ui/property_drawers.hpp"

namespace sanity::engine::renderer {
    void draw_component_properties(StandardRenderableComponent& renderable) {
        ui::draw_property("Mesh", renderable.mesh);
        ui::draw_property("Material", renderable.material);
        ui::draw_property("Is background", renderable.is_background);
    }

    void draw_component_properties(PostProcessingPassComponent& post_processing) {
        ui::draw_property("Draw Index", post_processing.draw_idx);
        ui::draw_property("Material", post_processing.material);
    }

    void draw_component_properties(RaytracingObjectComponent& raytracing_object) {
        ImGui::LabelText("Handle", "%#010x", raytracing_object.as_handle.index);
    }

    void draw_component_properties(CameraComponent& camera) {
        ui::draw_property("FOV", camera.fov);
        ui::draw_property("Aspect Ratio", camera.aspect_ratio);
        ui::draw_property("Near clip plane", camera.near_clip_plane);

        if(camera.fov <= 0) {
            ui::draw_property("Orthographic size", camera.orthographic_size);
            camera.fov = 0;
        }
    }

    void draw_component_properties(LightComponent& light) {
        ui::draw_property("Type", light.type);
        ui::draw_property("Color", light.color);
        ui::draw_property("Size", light.size);
    }

    void draw_component_properties(SkyComponent& sky) {
        // Fill in when the component has controls to draw
    }

    void draw_component_properties(FluidVolumeComponent& volume) {
        // TODO: Set the resolution of the volume and other fluid properties, update the GPU representation when the data changes
        ui::draw_property("Size", volume.volume->size);
    }
} // namespace sanity::engine::renderer
