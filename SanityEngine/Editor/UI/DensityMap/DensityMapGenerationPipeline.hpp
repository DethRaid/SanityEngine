#pragma once

#include "imgui_node_editor.h"
#include "../NodeGraphNode.hpp"
#include "rx/core/ptr.h"
#include "rx/core/vector.h"

using ax::NodeEditor::EditorContext;

namespace Sanity::Editor::UI::DensityMap {
    /*!
     * \brief Editor for a density map generation pipeline
     */
    class DensityMapGenerationPipeline
    {
    public:
        explicit DensityMapGenerationPipeline();
    	
    	~DensityMapGenerationPipeline();

void draw();
    	
    private:
        EditorContext* context = nullptr;

    	Rx::Vector<Rx::Ptr<NodeGraphNode>> nodes;
    };
}
