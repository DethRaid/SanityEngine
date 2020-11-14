#include "WorldgenParamsEditor.hpp"

#include "imgui/imgui.h"
#include "rx/core/log.h"

RX_LOG("WorldgenParamsEditor", logger);

namespace sanity::editor::ui {
    WorldgenParamsEditor::WorldgenParamsEditor() : Window{"Worldgen Params Editor"} {}

    void WorldgenParamsEditor::draw_contents() {
        // TODO: Have some kind of reference to the Sanity Editor. Get the current worldgen params, let the user change
        // them, then re-generate the world if needed

        if(ImGui::Button("Save worldgen params")) {
            logger->info("Saving worldgen params");
        }
    }
} // namespace sanity::editor::ui
