#pragma once

#include <rx/core/string.h>

class RexProfilerAdapter {
public:
    void set_thread_name(const rx::string& new_thread_name);
    void begin_sample(const rx::string& tag);
    void end_sample();

private:
    rx::string thread_name;

    rx::vector<rx::string> tag_stack;
};
