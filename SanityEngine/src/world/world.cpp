#include "world.hpp"

#include "actor/actor.hpp"
#include "entt/entity/registry.hpp"
#include "loading/image_loading.hpp"
#include "renderer/render_components.hpp"
#include "rx/core/log.h"
#include "sanity_engine.hpp"

namespace sanity::engine {
    RX_LOG("World", logger);

    World::World(entt::registry& registry_in) : registry{&registry_in} {
        auto& sky_actor = create_actor(*registry, "Sky");
        sky_actor.add_component<renderer::SkyboxComponent>();
        sky_actor.add_component<renderer::LightComponent>();
    }

    void World::set_skybox(const std::filesystem::path& skybox_image_path) {
        auto& atmosphere = registry->get<renderer::SkyboxComponent>(sky);

        // TODO: AssetStreamingManager that handles loading the asset if needed

        if(const auto* skybox_handle = cached_skybox_handles.find(skybox_image_path); skybox_handle != nullptr) {
            atmosphere.skybox_texture = *skybox_handle;
            logger->verbose("Using existing texture %d for skybox image %s", skybox_handle->index, skybox_image_path);

        } else {
            auto& renderer = g_engine->get_renderer();
            const auto handle = load_image_to_gpu(skybox_image_path, renderer);
            if(!handle) {
                return;
            }

            logger->verbose("Uploaded texture %d for skybox image %s", handle->index, skybox_image_path);
            cached_skybox_handles.insert(skybox_image_path, *handle);

            atmosphere.skybox_texture = *handle;
        }
    }
} // namespace sanity::engine
