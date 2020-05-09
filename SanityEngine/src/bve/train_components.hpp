#pragma once
#include "../rhi/raytracing_structs.hpp"

/*!
 * \brief Component with all the data that a train needs
 */
struct TrainComponent {
    /*!
     * \brief the raytracing mesh that represents this train
     */
    rhi::RaytracingMesh raytracing_mesh;

    // TODO: Add some information about the submeshes in this train
};
