#pragma once

#include <entt/entity/fwd.hpp>

namespace Rx {
    struct String;
}

namespace renderer {
    class Renderer;
}

bool load_static_mesh(const Rx::String& filename, entt::registry& registry, renderer::Renderer& renderer);
