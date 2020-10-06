#pragma once

#include <list>

#include "core/Prelude.hpp"
#include "core/types.hpp"

enum class FramerateDisplayMode { FrameTime, FramesPerSecond, Both };

struct FrametimeStats {
    double average;
    double minimum;
    double maximum;
};

class SANITY_API FramerateTracker {
public:
    explicit FramerateTracker(Uint32 max_num_samples_in);

    void add_frame_time(double frame_time);

    void log_framerate_stats(FramerateDisplayMode display_mode = FramerateDisplayMode::FrameTime) const;

    [[nodiscard]] FrametimeStats calculate_frametime_stats() const;

private:
    Uint32 max_num_samples;

    std::list<double> frame_times;
};
