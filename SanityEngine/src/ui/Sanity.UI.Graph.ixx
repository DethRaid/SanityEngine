module;

#include "imgui_node_editor.h"

export module Sanity.UI.Graph;

namespace Sanity::UI::Graph {
    /// <summary>
    /// Base class for nodes in a node editor GUI
    /// </summary>
    export class Node {
    public:
        void Draw();

    protected:
        virtual void DrawSelf() = 0;

    private:
        static int unique_id;
    };
} // namespace Sanity::UI::Graph

module :private;

namespace ed = ax::NodeEditor;

namespace Sanity::UI::Graph {

    int Node::unique_id = 1;

    void Node::Draw() {
        ed::BeginNode(unique_id++);

        DrawSelf();

        ed::EndNode();
    }
} // namespace Sanity::UI::Graph
