#include <string.h> // memcpy

#include "rx/core/serialize/encoder.h"
#include "rx/core/stream.h"

namespace Rx::serialize {

Encoder::Encoder(Memory::Allocator& _allocator, Stream* _stream)
  : m_allocator{_allocator}
  , m_stream{_stream}
  , m_buffer{m_stream, Buffer::Mode::k_write}
  , m_message{allocator()}
  , m_strings{allocator()}
{
  RX_ASSERT(m_stream->can_seek(), "encoder requires seekable stream");

  // Write out the default header, we'll seek back to patch it later.
  RX_ASSERT(write_header(), "failed to write header");
}

Encoder::~Encoder() {
  RX_ASSERT(finalize(), "finalization failed");
}

bool Encoder::write_uint(Uint64 _value) {
  // Encode |_value| using ULEB128 encoding.
  do {
    const Byte byte = _value & 0x7f;

    _value >>= 7;

    if (!m_buffer.write_byte(_value ? (byte | 0x80) : byte)) {
      return error("write failed");
    }
  } while (_value);

  return true;
}

bool Encoder::write_sint(Sint64 _value) {
  // Encode |_value| using SLEB128 encoding.
  bool more;

  do {
    const Byte byte = _value & 0x7f;
    const Byte test = byte & 0x40;

    // This assumes signed shift behaves as arithmetic right shift.
    _value >>= 7;

    more = !((_value == 0 && test == 0) || (_value == -1 && test != 0));

    if (!m_buffer.write_byte(more ? (byte | 0x80) : byte)) {
      return error("write failed");
    }
  } while (more);

  return true;
}

bool Encoder::write_float(Float32 _value) {
  return m_buffer.write_bytes(reinterpret_cast<const Byte*>(&_value), sizeof _value);
}

bool Encoder::write_bool(bool _value) {
  return m_buffer.write_byte(static_cast<Byte>(_value));
}

bool Encoder::write_byte(Byte _value) {
  return m_buffer.write_byte(_value);
}

bool Encoder::write_string(const char* _string, Size _size) {
  if (_string[_size] != '\0') {
    return error("string isn't null-terminated");
  }

  if (auto insert = m_strings.insert(_string, _size)) {
    return write_uint(*insert);
  }

  return false;
}

bool Encoder::write_float_array(const Float32* _data, Size _count) {
  if (!write_uint(_count)) {
    return false;
  }

  for (Size i = 0; i < _count; i++) {
    if (!write_float(_data[i])) {
      return false;
    }
  }

  return true;
}

bool Encoder::write_byte_array(const Byte* _data, Size _count) {
  if (!write_uint(_count)) {
    return false;
  }

  return m_buffer.write_bytes(_data, _count);
}

bool Encoder::write_header() {
  const auto header_data = reinterpret_cast<const Byte*>(&m_header);
  const auto output_size = m_stream->write(header_data, sizeof m_header);
  if (output_size != sizeof m_header) {
    return error("write failed");
  }
  return true;
}

bool Encoder::finalize() {
  // Flush anything remaining data in |m_buffer| out to |m_stream|.
  if (!m_buffer.flush()) {
    return error("flush failed");
  }

  // Update header fields.
  m_header.data_size = m_stream->tell() - sizeof m_header;
  m_header.string_size = m_strings.size();

  // Write out string table as the final thing in the stream.
  const auto string_table_data = reinterpret_cast<const Byte*>(m_strings.data());
  const auto string_table_size = m_strings.size();
  if (m_stream->write(string_table_data, string_table_size) != string_table_size) {
    return error("write failed");
  }

  // Seek to the beginning of the stream to update the header.
  if (!m_stream->seek(0, Stream::Whence::SET)) {
    return error("seek failed");
  }

  return write_header();
}

} // namespace rx::serialize
