#ifndef RX_CORE_TRAITS_REMOVE_EXTENT_H
#define RX_CORE_TRAITS_REMOVE_EXTENT_H
#include "rx/core/types.h" // Size
#include "rx/core/traits/type_identity.h"

namespace rx::traits {

namespace detail {
  template<typename T>
  struct remove_extent : traits::type_identity<T> {};

  template<typename T>
  struct remove_extent<T[]> : traits::type_identity<T> {};

  template<typename T, Size E>
  struct remove_extent<T[E]> : traits::type_identity<T> {};
} // namespace detail

template<typename T>
using remove_extent = typename detail::remove_extent<T>::type;

} // namespace rx::traits

#endif // RX_CORE_TRAITS_REMOVE_EXTENT_H
