#include "framerate_tracker.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>

#include "../core/ensure.hpp"

FramerateTracker::FramerateTracker(const uint32_t max_num_samples_in)
    : max_num_samples{max_num_samples_in}, logger{spdlog::stdout_color_st("FramerateTracker")} {
    ENSURE(max_num_samples_in > 0, "Must allow more than 0 frame time samples");
}

void FramerateTracker::add_frame_time(const double frame_time) {
    while(frame_times.size() > max_num_samples - 1) {
        frame_times.pop_back();
    }

    frame_times.push_front(frame_time);
}

void FramerateTracker::log_framerate_stats(FramerateDisplayMode display_mode) const {
    double min_time{100000000};
    double max_time{0};
    double average{0};

    for(const auto sample : frame_times) {
        min_time = std::min(sample, min_time);
        max_time = std::max(max_time, sample);
        average += sample;
    }

    average /= frame_times.size();

    switch(display_mode) {
        case FramerateDisplayMode::FrameTime:
            logger->info("Frame times: Avg: {:.3f} ms Min: {:.3f} ms Max: {:.3f} ms", average * 1000, min_time * 1000, max_time * 1000);
            break;
        case FramerateDisplayMode::FramesPerSecond:
            logger->info("Frames per second: Avg: {:.1f} Min: {:.1f} Max: {:.1f}", 1.0 / average, 1.0 / min_time, 1.0 / max_time);
            break;
        case FramerateDisplayMode::Both:
            logger->info("Frame times: Avg: {:.3f} ms ({:.3f} fps) Min: {:.3f} ms ({:.3f} fps) Max: {:.3f} ms ({:.3f} fps)",
                         average * 1000,
                         1.0 / average,
                         min_time * 1000,
                         1.0 / min_time,
                         max_time * 1000,
                         1.0 / max_time);
            break;
    }
}
