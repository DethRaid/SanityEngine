#pragma once

namespace sanity::editor {
    class SanityEditor {
    public:
        SanityEditor(const char* executable_directory);

        void run_until_quit() const;

    private:
        void create_editor_ui() const;
    };
} // namespace sanity::editor
