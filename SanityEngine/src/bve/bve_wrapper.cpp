#include "bve_wrapper.hpp"

BveWrapper::BveWrapper() { bve::bve_init(); }

BveMeshHandle BveWrapper::load_mesh_from_file(const std::string& filename) {
    auto* mesh = bve::bve_load_mesh_from_file(filename.c_str());

    return BveMeshHandle{mesh, bve::bve_delete_loaded_static_mesh};
}
