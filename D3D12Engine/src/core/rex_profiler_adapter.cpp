#include "rex_profiler_adapter.hpp"

#include <minitrace.h>

void RexProfilerAdapter::set_thread_name(const rx::string& new_thread_name) { thread_name = new_thread_name; }

void RexProfilerAdapter::begin_sample(const rx::string& tag) {
    MTR_BEGIN(thread_name.data(), tag.data());

    tag_stack.push_back(tag);
}

void RexProfilerAdapter::end_sample() {
    const auto& tag = tag_stack.last();
    MTR_END(thread_name.data(), tag.data());

    tag_stack.pop_back();
}
