#include "EngineTextureNode.hpp"

#include "imgui.h"
#include "UI/PropertyEditors.hpp"

namespace Sanity::Editor::UI::DensityMap {
    EngineTextureNode::EngineTextureNode(const renderer::Renderer& renderer_in) : renderer{renderer_in} {}

    void EngineTextureNode::draw_self() {	    
    	
    	ImGui::Text("Sample Engine Texture");

    	selected_texture = draw_enum_property(selected_texture);

    }
} // namespace Sanity::Editor::UI::DensityMap
