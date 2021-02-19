#pragma once

#include <filesystem>

#include "entt/fwd.hpp"
#include "renderer/rhi/resources.hpp"
#include "rx/core/map.h"

namespace Rx {
    struct String;
}

namespace sanity::engine {
	namespace renderer {
		class Renderer;
	}

	struct Actor;
    /*!
     * @brief Abstraction over the world
     *
     * Initial version: Manages the sun, moon, stars, and atmosphere
     *
     * Next version: Terrain, probably
     */
    class World {
    public:
        explicit World(entt::registry& registry_in);

    	void create_planetary_sky(renderer::Renderer& renderer);

        void set_skybox(const std::filesystem::path& skybox_image_path);

        Actor& create_actor(const Rx::String& name) const;  // NOLINT(modernize-use-nodiscard)

    	[[nodiscard]] Actor get_actor(const entt::entity& entity);

    private:
        entt::registry* registry;

        entt::entity sky;

        /*!
         * @brief All the skybox images currently on the GPU
         *
         * TODO: Some kind of "max allotted skybox memory" variable. If all the skyboxes together use up more than that much memory, the
         * least recently used one gets booted from VRAM
         */
        Rx::Map<std::filesystem::path, renderer::TextureHandle> cached_skybox_handles;
    };
} // namespace sanity::engine
