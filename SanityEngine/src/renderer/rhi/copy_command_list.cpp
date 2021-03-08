#include "copy_command_list.hpp"

#include "render_backend.hpp"
#include "rx/core/utility/move.h"

namespace sanity::engine::renderer {
    CopyCommandList::CopyCommandList(RenderBackend& backend_in, ComPtr<ID3D12GraphicsCommandList4> cmds_in)
        : backend{&backend_in}, cmds{Rx::Utility::move(cmds_in)} {}

    CopyCommandList::~CopyCommandList() {
        if(cmds) {
            cmds->Close();

            backend->submit_async_copy_commands(cmds);
        }
    }

    CopyCommandList::CopyCommandList(CopyCommandList&& old) noexcept {
        this->~CopyCommandList();

        backend = old.backend;
        cmds = old.cmds;

        old.cmds = {};
    }

    CopyCommandList& CopyCommandList::operator=(CopyCommandList&& old) noexcept {
        this->~CopyCommandList();

        backend = old.backend;
        cmds = old.cmds;

        old.cmds = {};

        return *this;
    }

    ID3D12GraphicsCommandList4* CopyCommandList::operator->() const { return *cmds; }

    ID3D12GraphicsCommandList4* CopyCommandList::operator*() const { return *cmds; }
} // namespace sanity::engine::renderer
