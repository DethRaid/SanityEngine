#include "rx/core/math/pow.h"
#include "rx/core/math/shape.h"

namespace Rx::Math {

Float32 pow(Float32 _x, Float32 _y) {
  // TODO(dweiler): implement a valid pow
  Shape u{static_cast<Float64>(_x)};
  const auto t1{static_cast<Sint32>(u.as_s64 >> 32)};
  const auto t2{static_cast<Sint32>(_y * (t1 - 1072632447) + 1072632447)};
  return static_cast<Float32>(Shape{static_cast<Sint64>(t2) << 32}.as_f64);
}

} // namespace rx::math
