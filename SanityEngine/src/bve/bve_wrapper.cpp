#include "bve_wrapper.hpp"

BveWrapper::BveWrapper() { bve::bve_init(); }

BveMeshHandle BveWrapper::load_mesh_from_file(const std::string& filename) {
    auto* mesh = bve::bve_load_mesh_from_file(filename.c_str());

    return BveMeshHandle{mesh, bve::bve_delete_loaded_static_mesh};
}

BveErrorDataHandle BveWrapper::get_printable_error(const bve::BVE_Mesh_Error& error) {
    auto* data = new bve::BVE_User_Error_Data;
    *data = bve::BVE_Mesh_Error_to_data(&error);

    return BveErrorDataHandle{data, [](bve::BVE_User_Error_Data* error_data) {
                                  bve::bve_delete_string(error_data->description);
                                  bve::bve_delete_string(error_data->description_english);

                                  delete error_data;
                              }};
}
