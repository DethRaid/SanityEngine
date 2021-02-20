#include "gpu_resource_pool.hpp"

namespace sanity::engine::renderer {
    Uint32 HandlePool::allocate_handle() {
        if(available_handles.is_empty()) {
            const auto handle = next_handle;
            next_handle++;
            return handle;
        }

        const auto handle = available_handles.last();
        available_handles.pop_back();
        return handle;
    }

    void HandlePool::free_handle(const Uint32 handle) { available_handles.push_back(handle); }
} // namespace sanity::engine::renderer
