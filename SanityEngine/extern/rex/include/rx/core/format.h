#ifndef RX_CORE_FORMAT_H
#define RX_CORE_FORMAT_H
#include <float.h> // {DBL,FLT}_MAX_10_EXP
#include <stdarg.h> // va_list, va_{start, copy, end}

#include "rx/core/utility/forward.h"
#include "rx/core/traits/remove_cvref.h"
#include "rx/core/hints/format.h"
#include "rx/core/types.h" // Size

namespace Rx {

// FormatSize holds maximum size needed to format a value of that Type
template<typename T>
struct FormatSize;

template<>
struct FormatSize<Float32> {
  static constexpr const Size size = 3 + FLT_MANT_DIG - FLT_MIN_EXP;
};

template<>
struct FormatSize<Float64> {
  static constexpr const Size size = 3 + DBL_MANT_DIG - DBL_MIN_EXP;
};

template<>
struct FormatSize<Sint32> {
  static constexpr const Size size = 3 + (8 * sizeof(Sint32) / 3);
};

template<>
struct FormatSize<Sint64> {
  static constexpr const Size size = 3 + (8 * sizeof(Sint64) / 3);
};

// FormatNormalize is used to convert non-trivial |T| into |...| compatible types.
template<typename T>
struct FormatNormalize {
  constexpr T operator()(const T& _value) const {
    return _value;
  }
};

template<Size E>
struct FormatNormalize<char[E]> {
  constexpr const char* operator()(const char (&_data)[E]) const {
    return _data;
  }
};

// Normalize for formatting.
template<typename T>
inline auto format_normalize(T&& _value) {
  return FormatNormalize<traits::remove_cvref<T>>{}(Utility::forward<T>(_value));
};

// Low-level format functions.
Size format_buffer_va_list(char* buffer_, Size _length, const char* _format, va_list _list);
Size format_buffer_va_args(char* buffer_, Size _length, const char* _format, ...) RX_HINT_FORMAT(3, 4);

template<typename... Ts>
inline Size format_buffer(char* buffer_, Size _length, const char* _format,
  Ts&&... _arguments)
{
  return format_buffer_va_args(buffer_, _length, _format,
    format_normalize(Utility::forward<Ts>(_arguments))...);
}

} // namespace rx

#endif // RX_CORE_FORMAT_H
