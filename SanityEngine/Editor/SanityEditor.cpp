#include "SanityEditor.hpp"

namespace Sanity::Editor {
    SanityEditor::SanityEditor()
    {
        Settings settings{};
    	
	    sanity_engine = std::make_unique<SanityEngine>(settings);
    }
} // namespace Sanity::Editor
