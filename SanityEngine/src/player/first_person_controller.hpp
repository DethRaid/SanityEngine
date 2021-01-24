#pragma once

#include "core/Prelude.hpp"
#include "core/async/synchronized_resource.hpp"
#include "core/types.hpp"
#include "entt/entity/fwd.hpp"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"

namespace sanity::engine {
    class Terrain;

    class PlatformInput;

    class FirstPersonController {
    public:
        explicit FirstPersonController(const PlatformInput& input_in,
                                       entt::entity controlled_entity_in,
                                       SynchronizedResource<entt::registry>& registry_in);

        void update_player_transform(Float32 delta_time);

        // void set_current_terrain(Terrain& terrain_in);

    private:
        Float32 normal_move_speed = 5;

        Float32 jump_velocity = 5;

        const PlatformInput& input;

        /*!
         * \brief The entity which represents the player
         */
        entt::entity controlled_entity;

        /*!
         * \brief Registry where all the player's components are stored
         */
        SynchronizedResource<entt::registry>* registry;

        Double2 last_cursor_location;

        glm::vec3 previous_location{};
        glm::vec3 velocity{0};

        // Terrain* terrain{nullptr};
        bool is_grounded{true};
    };
} // namespace sanity::engine
