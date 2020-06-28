#ifndef RX_CORE_MATH_FORCE_EVAL_H
#define RX_CORE_MATH_FORCE_EVAL_H
#include "rx/core/types.h"

namespace Rx {

inline void force_eval_f32(Float32 _x) {
  [[maybe_unused]] volatile Float32 y;
  y = _x;
}

inline void force_eval_f64(Float64 _x) {
  [[maybe_unused]] volatile Float64 y;
  y = _x;
}

} // namespace Rx

#endif // RX_CORE_MATH_FORCE_EVAL_H
