#include "stdoutstream.hpp"

#include <cstdio>

StdoutStream::StdoutStream() : rx::stream(k_flush | k_write) {}

rx_u64 StdoutStream::on_write(const rx_byte* data, const rx_u64 size) {
    fwrite(data, size, 1, stdout);
    return size;
}

bool StdoutStream::on_flush() {
    fflush(stdout);
    return true;
}

const rx::string& StdoutStream::name() const& { return my_name; }
