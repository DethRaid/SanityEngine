#ifndef RX_FOUNDATION_TYPES_H
#define RX_FOUNDATION_TYPES_H
#include "rx/core/config.h"
#include "rx/core/traits/conditional.h"

namespace Rx {

using Size = decltype(sizeof 0);
using Bool = bool;
using Byte = unsigned char;
using Sint8 = signed char;
using Uint8 = unsigned char;
using Sint16 = signed short;
using Uint16 = unsigned short;
using Sint32 = signed int;
using Uint32 = unsigned int;
using Sint64 = Rx::traits::conditional<sizeof(signed long) == 8, signed long, signed long long>;
using Uint64 = Rx::traits::conditional<sizeof(unsigned long) == 8, unsigned long, unsigned long long>;
using Float32 = float;
using Float64 = double;
using PtrDiff = decltype((Byte*)0 - (Byte*)0);
using UintPtr = Size;
using NullPointer = decltype(nullptr);
using Float32Eval = Float32;
using Float64Eval = Float64;

constexpr Size operator"" _z(unsigned long long _value) {
  return static_cast<Size>(_value);
}

constexpr Uint8 operator"" _u8(unsigned long long _value) {
  return static_cast<Uint8>(_value);
}

constexpr Uint16 operator"" _u16(unsigned long long _value) {
  return static_cast<Uint16>(_value);
}

constexpr Uint32 operator"" _u32(unsigned long long _value) {
  return static_cast<Uint32>(_value);
}

constexpr Uint64 operator"" _u64(unsigned long long _value) {
  return static_cast<Uint64>(_value);
}

} // namespace Rx

#endif // RX_FOUNDATION_TYPES_H
