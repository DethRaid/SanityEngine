#ifndef RX_CORE_SERIALIZE_ENCODER_H
#define RX_CORE_SERIALIZE_ENCODER_H
#include "rx/core/serialize/header.h"
#include "rx/core/serialize/buffer.h"

#include "rx/core/traits/is_signed.h"
#include "rx/core/traits/is_unsigned.h"

#include "rx/core/string.h"
#include "rx/core/string_table.h"

namespace Rx {
struct Stream;
} // namespace rx

namespace Rx::serialize {

struct Encoder {
  Encoder(Stream* _stream);
  Encoder(Memory::Allocator& _allocator, Stream* _stream);
  ~Encoder();

  [[nodiscard]] bool write_uint(Uint64 _value);
  [[nodiscard]] bool write_sint(Sint64 _value);
  [[nodiscard]] bool write_float(Float32 _value);
  [[nodiscard]] bool write_bool(bool _value);
  [[nodiscard]] bool write_byte(Byte _value);
  [[nodiscard]] bool write_string(const char* _string, Size _size);
  [[nodiscard]] bool write_string(const String& _string);

  [[nodiscard]] bool write_float_array(const Float32* _value, Size _count);
  [[nodiscard]] bool write_byte_array(const Byte* _data, Size _size);

  template<typename T>
  [[nodiscard]] bool write_uint_array(const T* _data, Size _count);

  template<typename T>
  [[nodiscard]] bool write_sint_array(const T* _data, Size _count);

  const String& message() const &;
  constexpr Memory::Allocator& allocator() const;

private:
  template<typename... Ts>
  bool error(const char* _format, Ts&&... _arguments);

  [[nodiscard]] bool write_header();
  [[nodiscard]] bool finalize();

  Memory::Allocator& m_allocator;
  Stream* m_stream;

  Header m_header;
  Buffer m_buffer;
  String m_message;
  StringTable m_strings;
};

inline Encoder::Encoder(Stream* _stream)
  : Encoder{Memory::SystemAllocator::instance(), _stream}
{
}

inline bool Encoder::write_string(const String& _string) {
  return write_string(_string.data(), _string.size());
}

template<typename T>
inline bool Encoder::write_uint_array(const T* _data, Size _count) {
  static_assert(traits::is_unsigned<T>, "T isn't unsigned integer Type");

  if (!write_uint(_count)) {
    return false;
  }

  for (Size i = 0; i < _count; i++) {
    if (!write_uint(_data[i])) {
      return false;
    }
  }

  return true;
}

template<typename T>
inline bool Encoder::write_sint_array(const T* _data, Size _count) {
  static_assert(traits::is_signed<T>, "T isn't signed integer Type");

  if (!write_uint(_count)) {
    return false;
  }

  for (Size i = 0; i < _count; i++) {
    if (!write_uint(_data[i])) {
      return false;
    }
  }

  return true;
}

inline const String& Encoder::message() const & {
  return m_message;
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Encoder::allocator() const {
  return m_allocator;
}

template<typename... Ts>
inline bool Encoder::error(const char* _format, Ts&&... _arguments) {
  m_message = String::format(allocator(), _format, Utility::forward<Ts>(_arguments)...);
  return false;
}

} // namespace rx::serialzie

#endif // RX_CORE_SERIALIZE_ENCODER_H
