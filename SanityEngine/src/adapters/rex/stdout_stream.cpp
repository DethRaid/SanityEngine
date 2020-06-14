#include "stdout_stream.hpp"

#include <cstdio>

namespace rex {
    StdoutStream::StdoutStream() : Stream(k_flush | k_write) {}

    Uint64 StdoutStream::on_write(const Byte* data, const Uint64 size) {
        fwrite(data, size, 1, stdout);
        return size;
    }

    bool StdoutStream::on_flush() {
        fflush(stdout);
        return true;
    }

    const Rx::String& StdoutStream::name() const& { return my_name; }

} // namespace rex
