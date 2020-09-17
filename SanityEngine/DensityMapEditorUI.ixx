export module DensityMapEditorUI;

import NodeEditorUi;

namespace sanity::ui {
    export class EngineTextureNode;
}

namespace sanity::ui {
    export class EngineTextureNode : public Node {
    public:
        void Draw() override;
    };

    void EngineTextureNode::Draw() {}
} // namespace sanity::ui
