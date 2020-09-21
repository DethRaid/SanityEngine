#include "DensityMapGenerationPipeline.hpp"

using namespace ax;

namespace Sanity::Editor::UI::DensityMap {
    DensityMapGenerationPipeline::DensityMapGenerationPipeline()
    {
        context = NodeEditor::CreateEditor();
    	
        nodes.reserve(128);
    }
	
    DensityMapGenerationPipeline::~DensityMapGenerationPipeline() { DestroyEditor(context); }
	
    void DensityMapGenerationPipeline::draw()
    { SetCurrentEditor(context);

	    NodeEditor::Begin("Density map generation pipeline editor");

    	nodes.each_fwd([&](const Rx::Ptr<NodeGraphNode>& node) { node->draw(); });

	    NodeEditor::End();
    }
} // namespace Sanity::Editor::UI::DensityMap
