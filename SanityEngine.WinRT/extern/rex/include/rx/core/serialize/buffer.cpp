#include <string.h> // memcpy

#include "rx/core/serialize/buffer.h"
#include "rx/core/stream.h"
#include "rx/core/algorithm/min.h"

namespace Rx::serialize {

Buffer::Buffer(Stream* _stream, Mode _mode)
  : m_stream{_stream}
  , m_mode{_mode}
  , m_cursor{0}
{
  switch (_mode) {
  case Mode::k_read:
    RX_ASSERT(_stream->can_read(), "buffer requires readable stream");
    break;
  case Mode::k_write:
    RX_ASSERT(_stream->can_write(), "buffer requires writable stream");
    break;
  }
}

Buffer::~Buffer() {
  switch (m_mode) {
  case Mode::k_read:
    RX_ASSERT(m_cursor == m_length, "data left in buffer");
    break;
  case Mode::k_write:
    RX_ASSERT(flush(), "flush failed");
    break;
  }
}

bool Buffer::write_byte(Byte _byte) {
  if (m_cursor == k_size && !flush()) {
    return false;
  }
  m_buffer[m_cursor++] = _byte;
  return true;
}

bool Buffer::read_byte(Byte* byte_) {
  if (m_cursor == m_length && !read()) {
    return false;
  }
  *byte_ = m_buffer[m_cursor++];
  return true;
}

bool Buffer::write_bytes(const Byte* _bytes, Size _size) {
  while (_size) {
    if (m_cursor == k_size && !flush()) {
      return false;
    }
    const auto max = Algorithm::min(_size, k_size - m_cursor);
    memcpy(m_buffer + m_cursor, _bytes, max);
    m_cursor += max;
    _bytes += max;
    _size -= max;
  }
  return true;
}

bool Buffer::read_bytes(Byte* bytes_, Size _size) {
  while (_size) {
    if (m_cursor == m_length && !read()) {
      return false;
    }
    const auto max = Algorithm::min(_size, m_length - m_cursor);
    memcpy(bytes_, m_buffer + m_cursor, max);
    m_cursor += max;
    bytes_ += max;
    _size -= max;
  }
  return true;
}

bool Buffer::flush() {
  const auto size = m_cursor;
  const auto bytes = m_stream->write(m_buffer, size);
  m_cursor = 0;
  return bytes == size;
}

bool Buffer::read(Uint64 _max_bytes) {
  _max_bytes = Algorithm::min(k_size, static_cast<Size>(_max_bytes));
  const auto bytes = m_stream->read(m_buffer, _max_bytes);
  m_cursor = 0;
  m_length = bytes;
  return m_length != 0;
}

} // namespace rx::serialize
