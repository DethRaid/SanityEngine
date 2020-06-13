#ifndef RX_CORE_ARRAY_H
#define RX_CORE_ARRAY_H
#include "rx/core/types.h"
#include "rx/core/assert.h"

#include "rx/core/utility/forward.h"

namespace Rx {

template<typename T>
struct Array;

template<typename T, Size E>
struct Array<T[E]> {
  template<typename... Ts>
  constexpr Array(Ts&&... _arguments);

  constexpr T& operator[](Size _index);
  constexpr const T& operator[](Size _index) const;

  constexpr T* data();
  constexpr const T* data() const;

  constexpr Size size() const;

private:
  T m_data[E];
};

// Deduction guide for Array{Ts...} to become Array<T[E]>.
template<typename T, typename... Ts>
Array(T, Ts...) -> Array<T[1 + sizeof...(Ts)]>;

template<typename T, Size E>
template<typename... Ts>
inline constexpr Array<T[E]>::Array(Ts&&... _arguments)
  : m_data{Utility::forward<Ts>(_arguments)...}
{
}

template<typename T, Size E>
inline constexpr T& Array<T[E]>::operator[](Size _index) {
  RX_ASSERT(_index < E, "out of bounds (%zu >= %zu)", _index, E);
  return m_data[_index];
}

template<typename T, Size E>
inline constexpr const T& Array<T[E]>::operator[](Size _index) const {
  RX_ASSERT(_index < E, "out of bounds (%zu >= %zu)", _index, E);
  return m_data[_index];
}

template<typename T, Size E>
inline constexpr T* Array<T[E]>::data() {
  return m_data;
}

template<typename T, Size E>
inline constexpr const T* Array<T[E]>::data() const {
  return m_data;
}

template<typename T, Size E>
inline constexpr Size Array<T[E]>::size() const {
  return E;
}

} // namespace rx

#endif // RX_CORE_ARRAY_H
