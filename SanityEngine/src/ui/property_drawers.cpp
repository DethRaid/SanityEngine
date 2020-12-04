#include "property_drawers.hpp"

#include "core/types.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "imgui/imgui.h"
#include "rx/core/string.h"

namespace sanity::engine::ui {
    void draw_property_editor(const Rx::String& label, glm::vec3& vec) {
        ImGui::PushID(label.data());

        ImGui::Text("%s", label.data());

        ImGui::SetCursorPosX(150);
        ImGui::PushItemWidth(50);
        ImGui::SameLine();
        ImGui::InputFloat("x", &vec.x);
        ImGui::SameLine();
        ImGui::InputFloat("y", &vec.y);
        ImGui::SameLine();
        ImGui::InputFloat("z", &vec.z);
        ImGui::PopItemWidth();

        ImGui::PopID();
    }

    void draw_property_editor(const Rx::String& label, glm::quat& quat) {
        ImGui::PushID(label.data());

        ImGui::Text("%s", label.data());

        ImGui::SetCursorPosX(150);
        ImGui::PushItemWidth(50);
        ImGui::SameLine();
        ImGui::InputFloat("x", &quat.x, 0, 0, "%.3f", ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine();
        ImGui::InputFloat("y", &quat.y, 0, 0, "%.3f", ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine();
        ImGui::InputFloat("z", &quat.z, 0, 0, "%.3f", ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine();
        ImGui::InputFloat("w", &quat.w, 0, 0, "%.3f", ImGuiInputTextFlags_ReadOnly);
        ImGui::PopItemWidth();

        ImGui::PopID();
    }

    void draw_property_editor(const Rx::String& label, Rx::String& string) {
        constexpr Size buffer_size = 1024;
        static char name_buffer[buffer_size];
    	
        memcpy(name_buffer, string.data(), Rx::Algorithm::max(buffer_size, string.size()));
       
        ImGui::InputText(label.data(), name_buffer, buffer_size / sizeof(char));
    	
        string = name_buffer;
    }
} // namespace sanity::engine::ui
