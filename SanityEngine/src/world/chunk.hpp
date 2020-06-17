#pragma once

#include <entt/entity/fwd.hpp>
#include <rx/core/vector.h>

#include "block_registry.hpp"
#include "core/types.hpp"

struct Chunk {
    // TODO: Is it worthwhile to try and make these cvars?
    static constexpr Int32 WIDTH{16};
    static constexpr Int32 HEIGHT{256};
    static constexpr Int32 DEPTH{16};

    enum class Status {
        BlockGenInProgress,

        BlockGenComplete,

        MeshGenInProgress,

        MeshGenComplete,
    };

    Status status{Status::BlockGenInProgress};

    Vec2i location;

    /*!
     * \brief All the blocks in this chunk
     *
     * This array takes up about a MB of space, meaning you need one MB of RAM for each chunk. This is probably fine - if it ends up not
     * being fine, I'll worry about it then
     */
    Rx::Vector<BlockId> block_data;

    entt::entity entity;

    /*!
     * \brief Ticks this chunk, updating simulated objects inside of it
     */
    void tick(Float32 delta_time);
};
