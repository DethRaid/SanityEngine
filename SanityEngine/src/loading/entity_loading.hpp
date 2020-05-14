#pragma once

#include <string>

#include <entt/entity/fwd.hpp>

namespace renderer {
    class Renderer;
}

bool load_static_mesh(const std::string& filename, entt::registry& registry, renderer::Renderer& renderer);
