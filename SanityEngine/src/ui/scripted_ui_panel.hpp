#pragma once

#include "scripting/entity_scripting_api.hpp"
#include "ui/ui_panel.hpp"

struct WrenHandle;

namespace script {
    class ScriptingRuntime;
}

namespace ui {
    class ScriptedUiPanel final : public UiPanel {
    public:
        explicit ScriptedUiPanel(WrenHandle* wren_handle_in, script::ScriptingRuntime& runtime_in);

        void draw() override;

    private:
        script::ScriptingRuntime* runtime;

        WrenHandle* wren_handle;

        WrenHandle* script_draw_method;
    };
}; // namespace ui
