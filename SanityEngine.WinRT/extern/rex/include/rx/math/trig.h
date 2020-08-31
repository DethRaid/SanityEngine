#ifndef RX_MATH_TRIG_H
#define RX_MATH_TRIG_H
#include "rx/core/types.h" // f32, f64
#include "rx/math/constants.h" // k_pi

namespace Rx::Math {

template<typename T>
inline constexpr T deg_to_rad(T deg) {
  return deg * k_pi<T> / T{180.0};
}

template<typename T>
inline constexpr T rad_to_deg(T rad) {
  return rad * T{180.0} / k_pi<T>;
}

} // namespace rx::math

#endif // RX_MATH_TRIG_H
