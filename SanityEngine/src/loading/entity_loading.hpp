#pragma once

#include <entt/entity/fwd.hpp>

#include "core/async/synchronized_resource.hpp"

namespace Rx {
    struct String;
}

namespace renderer {
    class Renderer;
}

bool load_static_mesh(const Rx::String& filename, SynchronizedResource<entt::registry>& registry, renderer::Renderer& renderer);
