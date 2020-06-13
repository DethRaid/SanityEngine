#ifndef RX_CORE_TRAITS_IS_SIGNED_H
#define RX_CORE_TRAITS_IS_SIGNED_H
#include "rx/core/traits/is_integral.h"

namespace Rx::traits {

template<typename T>
inline constexpr bool is_signed = is_integral<T> && T(-1) < T(0);

} // namespace rx::traits

#endif // RX_CORE_TRAITS_IS_SIGNED_H
