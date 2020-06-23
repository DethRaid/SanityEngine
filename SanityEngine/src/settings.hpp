#pragma once

enum class QualityLevel { Low, Medium, High, Ultra };

struct Settings {

    /*!
     * \brief Enables tracking GPU progress to debug GPU crashes
     *
     * This has a significant performance cost, so it should only be enabled when you know you need it
     */
    bool enable_gpu_crash_reporting{false};

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
