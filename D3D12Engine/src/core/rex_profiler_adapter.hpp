#pragma once

#include <rx/core/string.h>

class RexProfilerAdapter {
public:
    void set_thread_name(const std::string& new_thread_name);
    void begin_sample(const std::string& tag);
    void end_sample();

private:
    std::string thread_name;

    std::vector<std::string> tag_stack;
};
