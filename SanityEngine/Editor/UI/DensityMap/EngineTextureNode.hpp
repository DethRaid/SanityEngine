#pragma once

#include "UI/NodeGraphNode.hpp"

#include "world/environment/Environment.hpp"

namespace renderer {
	class Renderer;
}

namespace Sanity::Editor::UI::DensityMap {

    /*!
     * \brief Reads one of the textures that SanityEngine automatically generates
     */
    class EngineTextureNode : public NodeGraphNode {
    public:
        explicit EngineTextureNode(const renderer::Renderer& renderer_in);
    	
        ~EngineTextureNode() override = default;

    protected:
        void draw_self() override;

    private:
        environment::BuiltinTexture selected_texture = environment::BuiltinTexture::TerrainHeight;

    	const renderer::Renderer& renderer;
    };
} // namespace Sanity::Editor::UI::DensityMap
