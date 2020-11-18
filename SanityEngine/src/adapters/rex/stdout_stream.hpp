#pragma once

#include <cstdio>

#include "core/types.hpp"
#include "rx/core/stream.h"
#include "rx/core/rex_string.h"

namespace rex {
    class StdoutStream final : public Rx::Stream {
    public:
        StdoutStream();

        ~StdoutStream() override;

        [[nodiscard]] Uint64 on_write(const Byte* data, Uint64 size, Uint64 offset) override;

        [[nodiscard]] const Rx::String& name() const& override;

    private:
        Rx::String my_name{"SanityEngineLogStream"};
        FILE* fileyboi{nullptr};
    };
} // namespace rex
