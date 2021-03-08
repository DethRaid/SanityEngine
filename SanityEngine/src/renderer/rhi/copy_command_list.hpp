#pragma once

#include <d3d12.h>

#include "core/types.hpp"

namespace sanity::engine::renderer {
    class RenderBackend;

    /**
     * @brief Simple abstraction for a command list that copies data between resources
     *
     * This class will submit the D3D12 command list to the backend when it's destructed. It's intended for small jobs like uploading a
     * texture's image data or copying vertices between buffers
     */
    class CopyCommandList {
    public:
        CopyCommandList(RenderBackend& backend_in, ComPtr<ID3D12GraphicsCommandList4> cmds_in);

        ~CopyCommandList();

        CopyCommandList(const CopyCommandList& other) = delete;
        CopyCommandList& operator=(const CopyCommandList& other) = delete;

        CopyCommandList(CopyCommandList&& old) noexcept;
        CopyCommandList& operator=(CopyCommandList&& old) noexcept;

        // TODO: Add high-level methods for copying data between resources. These methods should save which resources are used. When the
        // RenderBackend receives a CopyCommandList, it looks at that list to know which resources need barriers on the direct command queue

        ID3D12GraphicsCommandList4* operator->() const;

        ID3D12GraphicsCommandList4* operator*() const;

    private:
        RenderBackend* backend{nullptr};
        ComPtr<ID3D12GraphicsCommandList4> cmds{};
    };
} // namespace sanity::engine::renderer
