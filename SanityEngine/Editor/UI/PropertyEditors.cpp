#include "PropertyEditors.hpp"

#include "imgui/imgui.h"

namespace Sanity::Editor::UI {
    Uint32 draw_enum_property(const Rx::Vector<Rx::String>& enum_values, Uint32 selected_idx) {
        if(ImGui::BeginCombo("Selected texture", enum_values[selected_idx].data())) {
            Uint32 cur_enum_value_idx = 0;
            enum_values.each_fwd([&](const Rx::String& enum_value) {
                const auto is_selected = cur_enum_value_idx == selected_idx;
                if(ImGui::Selectable(enum_value.data(), is_selected)) {
                    selected_idx = cur_enum_value_idx;
                }
                if(is_selected) {
                    ImGui::SetItemDefaultFocus();
                }

                cur_enum_value_idx++;
            });

            ImGui::EndCombo();
        }

        return selected_idx;
    }
} // namespace Sanity::Editor::UI
