#pragma once

#include <cstdint>
#include <memory>
#include <list>

#include <spdlog/logger.h>

enum class FramerateDisplayMode {
    FrameTime,
    FramesPerSecond,
    Both
};

class FramerateTracker {
public:
    explicit FramerateTracker(uint32_t max_num_samples_in);

    void add_frame_time(double frame_time);

    void log_framerate_stats(FramerateDisplayMode display_mode = FramerateDisplayMode::FrameTime) const;

private:
    uint32_t max_num_samples;

    std::list<double> frame_times;

    std::shared_ptr<spdlog::logger> logger;
};
