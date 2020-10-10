#include "SanityEditor.hpp"

namespace Sanity::Editor {
    SanityEditor::SanityEditor() {
        auto* window = glfwCreateWindow(640, 480, "Sanity Engine", nullptr, nullptr);
    	
        Settings settings{};

        sanity_engine = std::make_unique<SanityEngine>(settings);
    }
} // namespace Sanity::Editor
