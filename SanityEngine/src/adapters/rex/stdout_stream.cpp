#include "stdout_stream.hpp"

#include <cstdio>

namespace rex {
    StdoutStream::StdoutStream() : Stream(WRITE), fileyboi{freopen("CON", "wb", stdout)} { setvbuf(stdout, nullptr, _IONBF, 0); }

    StdoutStream::~StdoutStream() { fclose(fileyboi); }

    Uint64 StdoutStream::on_write(const Byte* data, const Uint64 size) {
        const auto result = fwrite(data, size, 1, fileyboi);
        return result == 1 ? size : ftell(fileyboi);
    }

    const Rx::String& StdoutStream::name() const& { return my_name; }

} // namespace rex
