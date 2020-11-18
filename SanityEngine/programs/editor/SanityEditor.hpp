#pragma once

#include "rx/core/ptr.h"
#include "sanity_engine.hpp"
#include "ui/ApplicationWindow.hpp"

namespace sanity::editor {
    class SanityEditor {
    public:
        SanityEditor();

        void run_until_quit();

    private:
        Rx::Ptr<ui::ApplicationWindow> main_window;

        Rx::Ptr<SanityEngine> engine;

        void create_editor_ui();
    };
} // namespace sanity::editor
