#pragma once

#include <functional>
#include <memory>

#include <bve/bve.hpp>
#include <entt/entity/fwd.hpp>
#include <spdlog/logger.h>

#include "../rhi/mesh_data_store.hpp"

namespace renderer {
    class Renderer;
}

using BveMeshHandle = std::unique_ptr<bve::BVE_Loaded_Static_Mesh, std::function<void(bve::BVE_Loaded_Static_Mesh*)>>;

class BveWrapper {
public:
    BveWrapper();

    [[nodiscard]] bool add_train_to_scene(const std::string& filename, entt::registry& registry, renderer::Renderer& renderer);

private:
    static std::shared_ptr<spdlog::logger> logger;

    [[nodiscard]] bve::BVE_User_Error_Data get_printable_error(const bve::BVE_Mesh_Error& error);

    [[nodiscard]] BveMeshHandle load_mesh_from_file(const std::string& filename);

    [[nodiscard]] std::pair<std::vector<BveVertex>, std::vector<uint32_t>> process_vertices(const bve::BVE_Mesh& mesh) const;
};