#include "property_drawers.hpp"

#include <vector>

#include "core/transform.hpp"
#include "core/types.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "imgui/imgui.h"
#include "renderer/hlsl/standard_material.hpp"
#include "renderer/lights.hpp"
#include "renderer/mesh.hpp"
#include "rx/core/string.h"

namespace sanity::engine::ui {
    void draw_property(const Rx::String& label, bool& b) { ImGui::Checkbox(label.data(), &b); }

    void draw_property(const Rx::String& label, Float32& f) { ImGui::InputFloat(label.data(), &f); }

    void draw_property(const Rx::String& label, Float64& f) { ImGui::InputDouble(label.data(), &f); }

    void draw_property(const Rx::String& label, Uint32& u) { ImGui::InputScalar(label.data(), ImGuiDataType_U32, &u); }

    void draw_property(const Rx::String& label, glm::vec3& vec) {
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

    void draw_property(const Rx::String& label, glm::uvec3& vec) {
        ImGui::PushID(label.data());

        ImGui::Text("%s", label.data());

        ImGui::SetCursorPosX(150);
        ImGui::PushItemWidth(75);
        ImGui::SameLine();
        ImGui::InputInt("x", reinterpret_cast<int*>(&vec.x));
        ImGui::SameLine();
        ImGui::InputInt("y", reinterpret_cast<int*>(&vec.y));
        ImGui::SameLine();
        ImGui::InputInt("z", reinterpret_cast<int*>(&vec.z));
        ImGui::PopItemWidth();

        ImGui::PopID();
    }

    void draw_property(const Rx::String& label, glm::quat& quat) {
        auto euler = eulerAngles(quat);

        ImGui::PushID(label.data());

        ImGui::Text("%s", label.data());

        ImGui::SetCursorPosX(150);
        ImGui::PushItemWidth(50);
        ImGui::SameLine();
        ImGui::InputFloat("Roll", &euler.z, 0, 0, "%.3f");
        ImGui::SameLine();
        ImGui::InputFloat("Pitch", &euler.x, 0, 0, "%.3f");
        ImGui::SameLine();
        ImGui::InputFloat("Yaw", &euler.y, 0, 0, "%.3f");
        ImGui::PopItemWidth();

        ImGui::PopID();

        quat = glm::quat(euler);
    }

    void draw_property(const Rx::String& label, Transform& transform) {
        ImGui::Text("%s", label.data());

        draw_property("location", transform.location);
        draw_property("rotation", transform.rotation);
        draw_property("scale", transform.scale);
    }

    void draw_property(const Rx::String& label, Rx::String& string) {
        constexpr Size buffer_size = 1024;
        static char name_buffer[buffer_size];

        memcpy(name_buffer, string.data(), Rx::Algorithm::max(buffer_size, string.size()));

        ImGui::InputText(label.data(), name_buffer, buffer_size / sizeof(char));

        string = name_buffer;
    }

    void draw_property(const Rx::String& label, renderer::Mesh& mesh) {
        ImGui::Text("%s", label.data());

        ImGui::LabelText("First vertex", "%d", mesh.first_vertex);
        ImGui::LabelText("Num vertices", "%d", mesh.num_vertices);
        ImGui::LabelText("First index", "%d", mesh.first_index);
        ImGui::LabelText("Num indices", "%d", mesh.num_indices);
    }

    void draw_property(const Rx::String& label, renderer::StandardMaterialHandle& handle) {
        ImGui::Text("%s", label.data());

        ImGui::LabelText("Handle", "%#010x", handle.index);
    }

    void draw_property(const Rx::String& label, renderer::LightType& type) {
        const static Rx::Vector<Rx::String> TYPE_NAMES = Rx::Array{"Directional", "Sphere"};
        
        draw_drop_down_selector(label, TYPE_NAMES, reinterpret_cast<Uint32&>(type));
    }

    void draw_property(const Rx::String& label, renderer::GpuLight& light) {
        ImGui::Text("%s", label.data());

        draw_property("Type", light.type);
        draw_property("Color", light.color);
        draw_property("Direction", light.direction_or_location);
        draw_property("Angular size", light.size);
    }

    void draw_drop_down_selector(const Rx::String& label, const Rx::Vector<Rx::String>& list_items, Uint32& selected_item) {
        if(ImGui::BeginCombo(label.data(), list_items[selected_item].data())) {
            for(auto i = 0u; i < list_items.size(); i++) {
                const auto is_selected = selected_item == i;
                if(ImGui::Selectable(list_items[i].data(), is_selected)) {
                    selected_item = i;
                }
                if(is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }
    }
} // namespace sanity::engine::ui
