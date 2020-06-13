#ifndef RX_CORE_FORMAT_H
#define RX_CORE_FORMAT_H
#include <float.h> // {DBL,FLT}_MAX_10_EXP

#include "rx/core/types.h" // rx_size

namespace Rx {

// format_size holds maximum size needed to format a value of that Type
template<typename T>
struct FormatSize;

template<>
struct FormatSize<Float32> {
  static constexpr const Size size{3 + FLT_MANT_DIG - FLT_MIN_EXP};
};

template<>
struct FormatSize<Float64> {
  static constexpr const Size size{3 + DBL_MANT_DIG - DBL_MIN_EXP};
};

template<>
struct FormatSize<Sint32> {
  static constexpr const Size size{3 + (8 * sizeof(Sint32) / 3)};
};

template<>
struct FormatSize<Sint64> {
  static constexpr const Size size{3 + (8 * sizeof(Sint64) / 3)};
};

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

} // namespace rx

#endif // RX_CORE_FORMAT_H
