#ifndef RX_CORE_TRAITS_IS_FLOATING_POINT
#define RX_CORE_TRAITS_IS_FLOATING_POINT
#include "rx/core/types.h" // Float{32,64}

#include "rx/core/traits/is_same.h"
#include "rx/core/traits/remove_cv.h"

namespace rx::traits {

template<typename T>
inline constexpr const bool is_floating_point{
  is_same<Float32, remove_cv<T>> ||
  is_same<Float64, remove_cv<T>>
};

} // namespace rx::traits

#endif // RX_CORE_TRAITS_IS_FLOATING_POINT
