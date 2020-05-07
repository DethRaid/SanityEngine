#pragma once

#include <functional>
#include <memory>

#include <bve/bve.hpp>

using BveMeshHandle = std::unique_ptr<bve::BVE_Loaded_Static_Mesh, std::function<void(bve::BVE_Loaded_Static_Mesh*)>>;

class BveWrapper {
public:
    BveWrapper();

    BveMeshHandle load_mesh_from_file(const std::string& filename);
};
