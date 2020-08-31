#ifndef RX_MATH_QUAT_H
#define RX_MATH_QUAT_H
#include "rx/core/types.h"

namespace Rx::Math {

template<typename T>
struct Mat3x3;

template<typename T>
struct Mat3x4;

template<typename T>
struct Quat {
  constexpr Quat(T _x, T _y, T _z, T _w);

  explicit Quat(const Mat3x3<T>& _rotation);
  explicit Quat(const Mat3x4<T>& _rotation);

  constexpr Quat operator-() const;

  constexpr Quat operator*(const Quat& _quat) const;
  constexpr Quat operator+(const Quat& _quat) const;
  constexpr Quat operator-(const Quat& _quat) const;

  constexpr Quat& operator*=(const Quat& _quat);
  constexpr Quat& operator+=(const Quat& _quat);
  constexpr Quat& operator-=(const Quat& _quat);

  constexpr Quat operator*(T _scalar) const;
  constexpr Quat operator+(T _scalar) const;
  constexpr Quat operator-(T _scalar) const;

  constexpr Quat& operator*=(T _scalar);
  constexpr Quat& operator+=(T _scalar);
  constexpr Quat& operator-=(T _scalar);

  T x, y, z, w;
};

using Quatf = Quat<Float32>;

template<typename T>
inline constexpr Quat<T>::Quat(T _x, T _y, T _z, T _w)
  : x{_x}
  , y{_y}
  , z{_z}
  , w{_w}
{
}

template<typename T>
inline constexpr Quat<T> Quat<T>::operator-() const {
  return {-x, -y, -z, w};
}

template<typename T>
inline constexpr Quat<T> Quat<T>::operator*(const Quat& _quat) const {
  return {w * _quat.x + x * _quat.w - y * _quat.z + z * _quat.y,
          w * _quat.y + x * _quat.z + y * _quat.w - z * _quat.x,
          w * _quat.z - x * _quat.y + y * _quat.x + z * _quat.w,
          w * _quat.w - x * _quat.x - y * _quat.y - z * _quat.z};
}

template<typename T>
inline constexpr Quat<T> Quat<T>::operator+(const Quat& _quat) const {
  return {x + _quat.x, y + _quat.y, z + _quat.z, w + _quat.w};
}

template<typename T>
inline constexpr Quat<T> Quat<T>::operator-(const Quat& _quat) const {
  return {x - _quat.x, y - _quat.y, z - _quat.z, w - _quat.w};
}

template<typename T>
inline constexpr Quat<T>& Quat<T>::operator*=(const Quat& _quat) {
  return *this = *this * _quat;
}

template<typename T>
inline constexpr Quat<T>& Quat<T>::operator+=(const Quat& _quat) {
  return *this = *this + _quat;
}

template<typename T>
inline constexpr Quat<T>& Quat<T>::operator-=(const Quat& _quat) {
  return *this = *this - _quat;
}

template<typename T>
inline constexpr Quat<T> Quat<T>::operator*(T _scalar) const {
  return {x * _scalar, y * _scalar, z * _scalar, w * _scalar};
}

template<typename T>
inline constexpr Quat<T> Quat<T>::operator+(T _scalar) const {
  return {x + _scalar, y + _scalar, z + _scalar, w + _scalar};
}

template<typename T>
inline constexpr Quat<T> Quat<T>::operator-(T _scalar) const {
  return {x - _scalar, y - _scalar, z - _scalar, w - _scalar};
}

template<typename T>
inline constexpr Quat<T>& Quat<T>::operator*=(T _scalar) {
  return *this = *this * _scalar;
}

template<typename T>
inline constexpr Quat<T>& Quat<T>::operator+=(T _scalar) {
  return *this = *this + _scalar;
}

template<typename T>
inline constexpr Quat<T>& Quat<T>::operator-=(T _scalar) {
  return *this = *this - _scalar;
}

template<typename T>
inline constexpr T dot(const Quat<T>& _lhs, const Quat<T>& _rhs) {
  return _lhs.x*_rhs.x + _lhs.y*_rhs.y + _lhs.z*_rhs.z + _lhs.w*_rhs.w;
}

Float32 length(const Quatf& _value);
Quatf normalize(const Quatf& _value);

} // namespace rx::math

#endif // RX_MATH_QUAT_H
