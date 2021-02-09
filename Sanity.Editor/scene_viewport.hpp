#pragma once

#include "ui/Window.hpp"

namespace sanity::editor::ui {
    class SceneViewport : public engine::ui::Window {
    public:
        explicit SceneViewport();

    	SceneViewport(const SceneViewport& other) = default;
        SceneViewport& operator=(const SceneViewport& other) = default;

    	SceneViewport(SceneViewport&& old) noexcept = default;
        SceneViewport& operator=(SceneViewport&& old) noexcept = default;
    	
        ~SceneViewport() override = default;
    
    protected:
	    void draw_contents() override;

        ImTextureID scene_output_texture{nullptr};
    };
} // namespace sanity::editor::ui
