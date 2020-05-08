#pragma once

#include <functional>
#include <memory>

#include <bve/bve.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

using BveMeshHandle = std::unique_ptr<bve::BVE_Loaded_Static_Mesh, std::function<void(bve::BVE_Loaded_Static_Mesh*)>>;

uint32_t to_uint32_t(const bve::BVE_Vector4<uint8_t>& bve_color);

glm::vec2 to_glm_vec2(const bve::BVE_Vector2<float>& bve_vec2);

glm::vec3 to_glm_vec3(const bve::BVE_Vector3<float>& bve_vec3);

class BveWrapper {
public:
    BveWrapper();

    BveMeshHandle load_mesh_from_file(const std::string& filename);

    bve::BVE_User_Error_Data get_printable_error(const bve::BVE_Mesh_Error& error);
};
