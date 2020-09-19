#include "NodeGraphNode.hpp"

#include <imgui_node_editor.h>

namespace ed = ax::NodeEditor;

namespace Sanity::Editor {
    void UI::NodeGraphNode::draw() {
        ed::BeginNode(unique_id++);

        draw_self();

        ed::EndNode();
    }
} // namespace Sanity::Editor
