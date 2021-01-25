#include "property_drawers.hpp"

#include "core/transform.hpp"
#include "core/types.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "imgui/imgui.h"
#include "renderer/handles.hpp"
#include "renderer/lights.hpp"
#include "renderer/mesh.hpp"
#include "rx/core/string.h"

namespace sanity::engine::ui {
    void draw_property_editor(const Rx::String& label, bool& b) { ImGui::Checkbox(label.data(), &b); }

    void draw_property_editor(const Rx::String& label, Float32& f) { ImGui::InputFloat(label.data(), &f); }

    void draw_property_editor(const Rx::String& label, Float64& f) { ImGui::InputDouble(label.data(), &f); }

    void draw_property_editor(const Rx::String& label, Uint32& u) { ImGui::InputScalar(label.data(), ImGuiDataType_U32, &u); }

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

    void draw_property_editor(const Rx::String& label, Transform& transform) {
        if(ImGui::CollapsingHeader(label.data())) {
            draw_property_editor("location", transform.location);
            draw_property_editor("rotation", transform.rotation);
            draw_property_editor("scale", transform.scale);
        }
    }

    void draw_property_editor(const Rx::String& label, Rx::String& string) {
        constexpr Size buffer_size = 1024;
        static char name_buffer[buffer_size];

        memcpy(name_buffer, string.data(), Rx::Algorithm::max(buffer_size, string.size()));

        ImGui::InputText(label.data(), name_buffer, buffer_size / sizeof(char));

        string = name_buffer;
    }

    void draw_property_editor(const Rx::String& label, renderer::Mesh& mesh) {
        if(ImGui::CollapsingHeader(label.data())) {
            ImGui::LabelText("First vertex", "%d", mesh.first_vertex);
            ImGui::LabelText("Num vertices", "%d", mesh.num_vertices);
            ImGui::LabelText("First index", "%d", mesh.first_index);
            ImGui::LabelText("Num indices", "%d", mesh.num_indices);
        }
    }

    void draw_property_editor(const Rx::String& label, renderer::StandardMaterialHandle& handle) {
        if(ImGui::CollapsingHeader(label.data())) {
            ImGui::LabelText("Handle", "%#010x", handle.index);
        }
    }

    void draw_property_editor(const Rx::String& label, renderer::LightType& type) {
        const auto type_names = new const char*[]{"Directional"};

        ImGui::ListBox(label.data(), reinterpret_cast<int*>(&type), type_names, 1);
    }

    void draw_property_editor(const Rx::String& label, renderer::Light& light) {
        ui::draw_property_editor("Type", light.type);
        ui::draw_property_editor("Color", light.color);
        ui::draw_property_editor("Direction", light.direction_or_location);
        ui::draw_property_editor("Angular size", light.angular_size);
    }
} // namespace sanity::engine::ui
