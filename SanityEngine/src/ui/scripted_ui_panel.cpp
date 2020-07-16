#include "scripted_ui_panel.hpp"

#include "rx/core/log.h"

namespace ui {
    // RX_LOG("ScriptedUiPanel", logger);
    // 
    // ScriptedUiPanel::ScriptedUiPanel(WrenHandle* wren_handle_in, script::ScriptingRuntime& runtime_in)
    //     : runtime{&runtime_in}, wren_handle{wren_handle_in}, script_draw_method{wrenMakeCallHandle(runtime->get_vm(), "draw()")} {}
    // 
    // void ScriptedUiPanel::draw() {
    //     auto* vm = runtime->get_vm();
    // 
    //     wrenEnsureSlots(vm, 1);
    //     wrenSetSlotHandle(vm, 0, wren_handle);
    // 
    //     wrenCall(vm, script_draw_method);
    // }
} // namespace ui
