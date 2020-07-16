#pragma once

#include "core/async/synchronized_resource.hpp"
#include "entt/entity/fwd.hpp"

namespace Rx {
    struct String;
}

namespace renderer {
    class Renderer;
}

bool load_static_mesh(const Rx::String& filename, SynchronizedResource<entt::registry>& registry, renderer::Renderer& renderer);
