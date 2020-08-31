#ifndef RX_CORE_MATH_SHAPE_H
#define RX_CORE_MATH_SHAPE_H
#include "rx/core/types.h"

namespace Rx::Math {

template<typename T>
union Shape;

template<>
union Shape<Float32> {
  constexpr Shape(Float32 _f);
  constexpr Shape(Uint32 _u);
  constexpr Shape(Sint32 _s);
  Float32 as_f32;
  Uint32 as_u32;
  Sint32 as_s32;
};

inline constexpr Shape<Float32>::Shape(Float32 _f)
  : as_f32{_f}
{
}

inline constexpr Shape<Float32>::Shape(Uint32 _u)
  : as_u32{_u}
{
}

inline constexpr Shape<Float32>::Shape(Sint32 _s)
  : as_s32{_s}
{
}

Shape(Uint32) -> Shape<Float32>;
Shape(Float32) -> Shape<Float32>;
Shape(Sint32) -> Shape<Float32>;

template<>
union Shape<Float64> {
  constexpr Shape(Float64 _f);
  constexpr Shape(Uint64 _u);
  constexpr Shape(Sint64 _s);
  Float64 as_f64;
  Uint64 as_u64;
  Sint64 as_s64;
};

inline constexpr Shape<Float64>::Shape(Float64 _f)
  : as_f64{_f}
{
}

inline constexpr Shape<Float64>::Shape(Uint64 _u)
  : as_u64{_u}
{
}

inline constexpr Shape<Float64>::Shape(Sint64 _s)
  : as_s64{_s}
{
}

Shape(Float64) -> Shape<Float64>;
Shape(Uint64) -> Shape<Float64>;
Shape(Sint64) -> Shape<Float64>;

} // namespace rx::math

#endif // RX_CORE_MATH_SHAPE_H