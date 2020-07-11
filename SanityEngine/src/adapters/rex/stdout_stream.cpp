#include "stdout_stream.hpp"

#include <cstdio>

namespace rex {
    StdoutStream::StdoutStream() : Stream(k_flush | k_write), fileyboi{freopen("CON", "wb", stdout)} {
    }

    StdoutStream::~StdoutStream() { fclose(fileyboi); }

    Uint64 StdoutStream::on_write(const Byte* data, const Uint64 size) {
        const auto result = fwrite(data, size, 1, fileyboi);
        return result == 1 ? size : ftell(fileyboi);
    }

    bool StdoutStream::on_flush() {
        fflush(fileyboi);
        return true;
    }

    const Rx::String& StdoutStream::name() const& { return my_name; }

} // namespace rex
