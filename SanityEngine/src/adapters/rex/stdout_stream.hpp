#pragma once

#include <rx/core/stream.h>
#include <rx/core/string.h>

namespace rex {
    class StdoutStream final : public Rx::Stream {
    public:
        StdoutStream();

        ~StdoutStream() override = default;

        [[nodiscard]] Uint64 on_write(const Byte* data, Uint64 size) override;

        [[nodiscard]] bool on_flush() override;

        [[nodiscard]] const Rx::String& name() const& override;

    private:
        Rx::String my_name = "SanityEngineLogStream";
    };
} // namespace rex
