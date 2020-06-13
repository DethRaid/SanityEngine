#ifndef RX_MATH_CONSTANTS_H
#define RX_MATH_CONSTANTS_H

namespace Rx::Math {

template<typename T>
inline constexpr const T k_pi{3.14159265358979323846264338327950288};

template<typename T>
inline constexpr const T k_tau{k_pi<T>*2.0};

template<typename T>
struct range {
  constexpr range(const T& _min, const T& _max);
  T min;
  T max;
};

template<typename T>
inline constexpr range<T>::range(const T& _min, const T& _max)
  : min{_min}
  , max{_max}
{
}

} // namespace rx

#endif //
