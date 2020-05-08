#include "bve_wrapper.hpp"

uint32_t to_uint32_t(const bve::BVE_Vector4<uint8_t>& bve_color) {
    uint32_t color{0};
    color |= bve_color.x;
    color |= bve_color.y << 8;
    color |= bve_color.z << 16;
    color |= bve_color.w << 24;

    return color;
}

glm::vec2 to_glm_vec2(const bve::BVE_Vector2<float>& bve_vec2) {
    return glm::vec2{bve_vec2.x, bve_vec2.y};
}

glm::vec3 to_glm_vec3(const bve::BVE_Vector3<float>& bve_vec3) {
    return glm::vec3{bve_vec3.x, bve_vec3.y, bve_vec3.z};
}

BveWrapper::BveWrapper() { bve::bve_init(); }

BveMeshHandle BveWrapper::load_mesh_from_file(const std::string& filename) {
    auto* mesh = bve::bve_load_mesh_from_file(filename.c_str());

    return BveMeshHandle{mesh, bve::bve_delete_loaded_static_mesh};
}

bve::BVE_User_Error_Data BveWrapper::get_printable_error(const bve::BVE_Mesh_Error& error) {
    return bve::BVE_Mesh_Error_to_data(&error);
}
