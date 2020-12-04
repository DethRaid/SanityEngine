#include "property_drawers.hpp"

#include "glm/glm.hpp"
#include "imgui/imgui.h"
#include "rx/core/string.h"

namespace sanity::engine::ui {
    void vec3_property(const Rx::String& label, glm::vec3& vec) {
        ImGui::Text("%s", label.data());
        ImGui::SameLine();
        ImGui::InputFloat("x", &vec.x);
        ImGui::SameLine();
        ImGui::InputFloat("y", &vec.y);
        ImGui::SameLine();
        ImGui::InputFloat("z", &vec.z);
    }

    void quat_property(const Rx::String& label, glm::quat& quat) { ImGui::Text("%s is a quat", label.data()); }
} // namespace sanity::engine::ui
