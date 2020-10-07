#include "stdout_stream.hpp"

#include <cstdio>

namespace rex {
    StdoutStream::StdoutStream() : Stream(k_flush | k_write) {}

    StdoutStream::~StdoutStream() { }

    Uint64 StdoutStream::on_write(const Byte* data, const Uint64 size) {
        const auto result = fwrite(data, size, 1, stdout);
        return result == 1 ? size : ftell(stdout);
    }

    bool StdoutStream::on_flush() {
        fflush(stdout);
        return true;
    }

    const Rx::String& StdoutStream::name() const& { return my_name; }

} // namespace rex
