#include "fps_display.hpp"

#include <imgui/imgui.h>

#include "../stats/framerate_tracker.hpp"
namespace ui {
    FramerateDisplay::FramerateDisplay(FramerateTracker& tracker_in) : UiPanel{}, tracker{&tracker_in} {}

    void FramerateDisplay::draw() {
        const auto [average, minimum, maximum] = tracker->calculate_frametime_stats();

        ImGui::Begin("Framerate");

        ImGui::Text("Average: %.3f ms (%.3f fps)", average, 1.0 / average);
        ImGui::Text("Minimum: %.3f ms (%.3f fps)", minimum, 1.0 / minimum);
        ImGui::Text("Maximum: %.3f ms (%.3f fps)", maximum, 1.0 / maximum);

        ImGui::End();
    }
} // namespace ui
