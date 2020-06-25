#pragma once

enum class QualityLevel { Low, Medium, High, Ultra };

struct Settings {
    /*!
     * \brief Scale of the internal render resolution relative to the screen resolution
     */
    Float32 render_scale{1.0f};

    /*!
     * \brief Quality to render the shadowmap at
     *
     * Gets translated into an actual resolution with a heurustic of the size of the main screen and the amount of available VRAM
     */
    QualityLevel shadow_quality;
};
