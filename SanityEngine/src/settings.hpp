#pragma once

enum class RenderQualityLevel { Low, Medium, High, Ultra, Custom };

struct Settings {
    /*!
     * \brief Scale of the internal render resolution relative to the screen resolution
     */
    Float32 render_scale{1.0f};

    /*!
     * \brief Overall quality to render at
     */
    RenderQualityLevel render_quality = RenderQualityLevel::Ultra;

    /*!
     * \brief Absolute path to the directory where the SanityEngine executable is
     */
    const char* executable_directory;
};
