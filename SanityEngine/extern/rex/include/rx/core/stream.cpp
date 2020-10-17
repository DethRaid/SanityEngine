#include <limits.h> // UCHAR_MAX
#include <string.h> // memmove

#include "rx/core/stream.h"
#include "rx/core/string.h" // utf16_to_utf8
#include "rx/core/abort.h"

#include "rx/core/hints/may_alias.h"

namespace Rx {

static Vector<Byte> convert_text_encoding(Vector<Byte>&& data_) {
  // Ensure the data contains a null-terminator.
  if (data_.last() != 0) {
    data_.push_back(0);
  }

  const bool utf16_le = data_.size() >= 2 && data_[0] == 0xFF && data_[1] == 0xFE;
  const bool utf16_be = data_.size() >= 2 && data_[0] == 0xFE && data_[1] == 0xFF;

  // UTF-16.
  if (utf16_le || utf16_be) {
    // Remove the BOM.
    data_.erase(0, 2);

    auto contents = reinterpret_cast<Uint16*>(data_.data());
    const Size chars = data_.size() / 2;
    if (utf16_be) {
      // Swap the bytes around in the contents to convert BE to LE.
      for (Size i = 0; i < chars; i++) {
        contents[i] = (contents[i] >> 8) | (contents[i] << 8);
      }
    }

    // Determine how many bytes are needed to convert the encoding.
    const Size length = utf16_to_utf8(contents, chars, nullptr);

    // Convert UTF-16 to UTF-8.
    Vector<Byte> result{data_.allocator(), length, Utility::UninitializedTag{}};
    utf16_to_utf8(contents, chars, reinterpret_cast<char*>(result.data()));
    return result;
  } else if (data_.size() >= 3 && data_[0] == 0xEF && data_[1] == 0xBB && data_[2] == 0xBF) {
    // Remove the BOM.
    data_.erase(0, 3);
  }

  return Utility::move(data_);
}

Uint64 Stream::on_read(Byte*, Uint64) {
  abort("stream does not implement on_read");
}

Uint64 Stream::on_write(const Byte*, Uint64) {
  abort("stream does not implement on_write");
}

bool Stream::on_seek(Uint64) {
  abort("stream does not implement on_seek");
}

bool Stream::on_stat(Stat&) const {
  abort("stream does not implement on_stat");
}

Uint64 Stream::read(Byte* _data, Uint64 _size) {
  const auto read = can_read() ? on_read(_data, _size) : 0;
  m_offset += read;
  return read;
}

Uint64 Stream::write(const Byte* _data, Uint64 _size) {
  const auto write = can_write() ? on_write(_data, _size) : 0;
  m_offset += write;
  return write;
}

bool Stream::seek(Sint64 _where, Whence _whence) {
  Uint64 offset = 0;

  // Calculate the new offset based on |_whence|
  if (_whence == Whence::CURRENT) {
    offset = m_offset + _where;
  } else if (_whence == Whence::SET) {
    offset = _where;
  } else if (_whence == Whence::END) {
    // Can only seek relative to END when we can stat.
    if (auto s = stat()) {
      offset = s->size + _where;
    } else {
      return false;
    }
  }

  if (on_seek(offset)) {
    m_offset = offset;
    return true;
  }

  return false;
}

Optional<Stream::Stat> Stream::stat() const {
  if (Stat s; can_stat() && on_stat(s)) {
    return s;
  }
  return nullopt;
}

Optional<Vector<Byte>> read_binary_stream(Memory::Allocator& _allocator, Stream* _stream) {
  const auto size = _stream->size();
  if (!size) {
    return nullopt;
  }

  Vector<Byte> result = {_allocator, static_cast<Size>(*size), Utility::UninitializedTag{}};
  if (_stream->read(result.data(), *size) == *size) {
    return result;
  }

  return nullopt;
}

Optional<Vector<Byte>> read_text_stream(Memory::Allocator& _allocator, Stream* _stream) {
  if (auto result = read_binary_stream(_allocator, _stream)) {
    // Convert the given byte stream into a compatible UTF-8 encoding. This will
    // introduce a null-terminator, strip Unicode BOMs and convert UTF-16
    // encodings to UTF-8.
    auto data = convert_text_encoding(Utility::move(*result));

#if defined(RX_PLATFORM_WINDOWS)
    // Quickly scan through word at a time |_src| for CR.
    auto scan = [](const void* _src, Size _size) {
      static constexpr auto k_ss = sizeof(Size);
      static constexpr auto k_align = k_ss - 1;
      static constexpr auto k_ones = -1_z / UCHAR_MAX; // All bits set.
      static constexpr auto k_highs = k_ones * (UCHAR_MAX / 2 + 1); // All high bits set.
      static constexpr auto k_c = static_cast<const Byte>('\r');
      static constexpr auto k_k = k_ones * k_c;

      auto has_zero = [&](Size _value) {
        return _value - k_ones & (~_value) & k_highs;
      };

      // Search for CR until |s| is aligned on |k_align| alignment.
      auto s = reinterpret_cast<const Byte*>(_src);
      auto n = _size;
      for (; (reinterpret_cast<UintPtr>(s) & k_align) && n && *s != k_c; s++, n--);

      // Do checks for CR word at a time, stopping at word containing CR.
      if (n && *s != k_c) {
        // Need to typedef with an alias Type since we're breaking strict
        // aliasing, let the compiler know.
        typedef Size RX_HINT_MAY_ALIAS WordType;

        // Scan word at a time, stopping at word containing first |k_c|.
        auto w = reinterpret_cast<const WordType*>(s);
        for (; n >= k_ss && !has_zero(*w ^ k_k); w++, n -= k_ss);
        s = reinterpret_cast<const Byte*>(w);
      }

      // Handle trailing bytes to determine where in word |k_c| is.
      for (; n && *s != k_c; s++, n--);

      return n ? s : nullptr;
    };

    const Byte* src = data.data();
    Byte* dst = data.data();
    Size size = data.size();
    auto next = scan(src, size);
    if (!next) {
      // Contents do not contain any CR.
      return data;
    }

    // Remove all instances of CR from the byte stream using instruction-level
    // parallelism.
    //
    // Note that the very first iteration will always have |src| == |dst|, so
    // optimize away the initial memmove here as nothing needs to be moved at
    // the beginning.
    const auto length = next - src;
    const auto length_plus_one = length + 1;
    dst += length;
    src += length_plus_one;
    size -= length_plus_one;

    // Subsequent iterations will move contents forward.
    while ((next = scan(src, size))) {
      const auto length = next - src;
      const auto length_plus_one = length + 1;
      memmove(dst, src, length);
      dst += length;
      src += length_plus_one;
      size -= length_plus_one;
    }

    // Ensure the result is null-terminated after all those moves.
    *dst++ = '\0';

    // Respecify the size of storage after removing all those CRs.
    data.resize(dst - data.data());
#endif
    return data;
  }

  return nullopt;
}

} // namespace rx
