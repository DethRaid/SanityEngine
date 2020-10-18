#pragma once

#include "rx/core/ptr.h"
#include "sanity_engine.hpp"

namespace sanity::editor {
    class SanityEditor {
    public:
        SanityEditor();
    	
        void run_until_quit();
    	
    private:
        Rx::Ptr<SanityEngine> engine = Rx::make_ptr<SanityEngine>(RX_SYSTEM_ALLOCATOR, R"(E:\Documents\SanityEngine\x64\Debug)");

        void create_editor_ui();
    };
} // namespace sanity::editor
