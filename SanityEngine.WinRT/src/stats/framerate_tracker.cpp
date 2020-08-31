#include "framerate_tracker.hpp"

#include <algorithm>

#include "rx/core/assert.h"
#include "rx/core/log.h"

RX_LOG("FramerateTracker", logger);

FramerateTracker::FramerateTracker(const Uint32 max_num_samples_in) : max_num_samples{max_num_samples_in} {
    RX_ASSERT(max_num_samples_in > 0, "Must allow more than 0 frame time samples");
}

void FramerateTracker::add_frame_time(const double frame_time) {
    while(frame_times.size() > max_num_samples - 1) {
        frame_times.pop_back();
    }

    frame_times.push_front(frame_time);
}

void FramerateTracker::log_framerate_stats(const FramerateDisplayMode display_mode) const {
    const auto [average, min_time, max_time] = calculate_frametime_stats();

    switch(display_mode) {
        case FramerateDisplayMode::FrameTime:
            logger->info("Frame times: Avg: %f.3 ms Min: %f.3 ms Max: %f.3 ms", average * 1000, min_time * 1000, max_time * 1000);
            break;
        case FramerateDisplayMode::FramesPerSecond:
            logger->info("Frames per second: Avg: {:.1f} Min: {:.1f} Max: {:.1f}", 1.0 / average, 1.0 / min_time, 1.0 / max_time);
            break;
        case FramerateDisplayMode::Both:
            logger->info("Frame times: Avg: %f.3 ms (%f.3 fps) Min: %f.3 ms (%f.3 fps) Max: %f.3 ms (%f.3 fps)",
                         average * 1000,
                         1.0 / average,
                         min_time * 1000,
                         1.0 / min_time,
                         max_time * 1000,
                         1.0 / max_time);
            break;
    }
}

FrametimeStats FramerateTracker::calculate_frametime_stats() const {
    double min_time{100000000};
    double max_time{0};
    double average{0};

    for(const auto sample : frame_times) {
        min_time = std::min(sample, min_time);
        max_time = std::max(max_time, sample);
        average += sample;
    }

    average /= frame_times.size();

    return {.average = average, .minimum = min_time, .maximum = max_time};
}
