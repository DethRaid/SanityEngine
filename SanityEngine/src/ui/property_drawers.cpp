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
        const static std::vector<const char*> TYPE_NAMES = {"Directional", "Sphere"};

        ImGui::ListBox(label.data(), reinterpret_cast<int*>(&type), TYPE_NAMES.data(), static_cast<int>(TYPE_NAMES.size()));
    }

    void draw_property_editor(const Rx::String& label, renderer::GpuLight& light) {
	    draw_property_editor("Type", light.type);
	    draw_property_editor("Color", light.color);
	    draw_property_editor("Direction", light.direction_or_location);
	    draw_property_editor("Angular size", light.size);
    }
} // namespace sanity::engine::ui
