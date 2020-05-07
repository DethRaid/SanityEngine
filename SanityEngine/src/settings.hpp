#pragma once

struct Settings {
    /*!
     * \brief Number of frames to submit tot he GPU before waiting for it to finish any of them
     */
    uint32_t num_in_flight_gpu_frames = 3;

    /*!
     * \brief Enables tracking GPU progress to debug GPU crashes
     *
     * This has a significant performance cost, so it should only be enabled when you know you need it
     */
    bool enable_gpu_crash_reporting{false};

    /*!
     * Scale of the internal render resolution relative to the screen resolution
     */
    float render_scale{1.0f};
};
