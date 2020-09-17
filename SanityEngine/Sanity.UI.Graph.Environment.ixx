export module Sanity.UI.Graph.Environment;

import Sanity.Environment;
import Sanity.UI.Graph;

namespace Sanity::UI::Graph::Environment {
    /// <summary>
    /// This node reads a builtin environment texture
    /// </summary>
    export class BuiltinEnvironmentTextureNode;
} // namespace Sanity::UI::Graph::Environment

namespace Sanity::UI::Graph::Environment {
    export class BuiltinEnvironmentTextureNode : public Node {
    public:
    protected:
        void DrawSelf() override;

    private:
        Sanity::Environment::BuiltinTextureType texture_type;
    };

    void BuiltinEnvironmentTextureNode::DrawSelf() {}
} // namespace Sanity::UI::Graph::Environment
