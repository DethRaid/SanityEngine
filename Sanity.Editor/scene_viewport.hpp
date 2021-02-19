#pragma once

#include <renderer/rhi/resources.hpp>

#include "core/types.hpp"
#include "ui/Window.hpp"

namespace sanity {
    namespace engine {
        namespace renderer {
            class Renderer;
        }
    } // namespace engine
} // namespace sanity

namespace sanity::editor::ui {
    class SceneViewport : public engine::ui::Window {
    public:
        explicit SceneViewport(engine::renderer::Renderer& renderer_in);

        SceneViewport(const SceneViewport& other) = default;
        SceneViewport& operator=(const SceneViewport& other) = default;

        SceneViewport(SceneViewport&& old) noexcept = default;
        SceneViewport& operator=(SceneViewport&& old) noexcept = default;

        ~SceneViewport() override = default;

        void set_render_size(const Uint2& new_render_size);

    protected:
        engine::renderer::Renderer* renderer;

        Uint2 render_size{0, 0};

        ImTextureID scene_output_texture{nullptr};
    	
        engine::renderer::TextureHandle scene_output_texture_handle;

        void draw_contents() override;

        void recreate_scene_output_texture();
    };
} // namespace sanity::editor::ui
