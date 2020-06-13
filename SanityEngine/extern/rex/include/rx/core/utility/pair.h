#ifndef RX_CORE_UTILITY_PAIR_H
#define RX_CORE_UTILITY_PAIR_H
#include "rx/core/utility/forward.h"

namespace Rx {

template<typename T1, typename T2>
struct Pair {
  constexpr Pair();
  constexpr Pair(const T1& _first, const T2& _second);

  template<typename U1, typename U2>
  constexpr Pair(U1&& first_, U2&& second_);

  template<typename U1, typename U2>
  constexpr Pair(const Pair<U1, U2>& _pair);

  template<typename U1, typename U2>
  constexpr Pair(Pair<U1, U2>&& pair_);

  T1 first;
  T2 second;
};

template<typename T1, typename T2>
inline constexpr Pair<T1, T2>::Pair()
  : first{}
  , second{}
{
}

template<typename T1, typename T2>
inline constexpr Pair<T1, T2>::Pair(const T1& _first, const T2& _second)
  : first{_first}
  , second{_second}
{
}

template<typename T1, typename T2>
template<typename U1, typename U2>
inline constexpr Pair<T1, T2>::Pair(U1&& first_, U2&& second_)
  : first{Utility::forward<U1>(first_)}
  , second{Utility::forward<U2>(second_)}
{
}

template<typename T1, typename T2>
template<typename U1, typename U2>
inline constexpr Pair<T1, T2>::Pair(const Pair<U1, U2>& _pair)
  : first{_pair.first}
  , second{_pair.second}
{
}

template<typename T1, typename T2>
template<typename U1, typename U2>
inline constexpr Pair<T1, T2>::Pair(Pair<U1, U2>&& pair_)
  : first{Utility::forward<U1>(pair_.first)}
  , second{Utility::forward<U2>(pair_.second)}
{
}

template<typename T1, typename T2>
Pair(T1, T2) -> Pair<T1, T2>;

} // namespace rx

#endif // RX_CORE_UTILITY_PAIR_H
