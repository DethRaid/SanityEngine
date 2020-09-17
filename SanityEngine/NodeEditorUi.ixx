export module NodeEditorUi;

#include "imgui_node_editor.h"

namespace sanity::ui {
    /// <summary>
    /// Base class for nodes in a node editor GUI
    /// </summary>
    export class Node;
} // namespace sanity::ui

namespace ed = ax::NodeEditor;

namespace sanity::ui {
    class Node {
    public:
        void Draw();

    protected:
        virtual void DrawSelf() = 0;

    private:
        static int unique_id;
    };

    int Node::unique_id = 1;

    void Node::Draw() {
        ed::BeginNode(unique_id++);

        DrawSelf();

        ed::EndNode();
    }
} // namespace sanity::ui
