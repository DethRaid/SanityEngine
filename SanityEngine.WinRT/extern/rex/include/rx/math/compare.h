#ifndef RX_MATH_COMPARE_H
#define RX_MATH_COMPARE_H
#include "rx/core/math/abs.h" // abs
#include "rx/core/algorithm/max.h" // max

namespace Rx::Math {

template<typename T>
inline constexpr const T k_epsilon = T(0.0001);

inline bool epsilon_compare(Float32 _x, Float32 _y) {
  return abs(_x - _y) <= k_epsilon<Float32> * Algorithm::max(1.0f, _x, _y);
}

} // namespace rx::math

#endif // RX_MATH_COMPARE_H
