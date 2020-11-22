#pragma once

#include "sanity_engine.hpp"

namespace sanity::editor {
    class SanityEditor {
    public:
        SanityEditor(const char* executable_directory);

        void run_until_quit() const;

    private:
        SanityEngine* engine{nullptr};

        void create_editor_ui() const;
    };
} // namespace sanity::editor
