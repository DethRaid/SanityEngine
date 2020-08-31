#ifndef RX_MATH_MAT3X4_H
#define RX_MATH_MAT3X4_H
#include "rx/math/vec4.h"

namespace Rx::Math {

template<typename T>
struct Quat;

template<typename T>
struct Mat3x3;

template<typename T>
struct Vec3;

template<typename T>
struct Mat3x4 {
  using Vec = Vec4<T>;

  constexpr Mat3x4() = default;
  constexpr Mat3x4(const Vec& _x, const Vec& _y, const Vec& _z);

  explicit Mat3x4(const Mat3x3<T>& _rotation, const Vec3<T>& _translation);
  explicit Mat3x4(const Quat<T>& _rotation, const Vec3<T>& _translation);
  explicit Mat3x4(const Vec3<T>& _scale, const Quat<T>& _rotation, const Vec3<T>& _translation);

  static Mat3x4 invert(const Mat3x4& _mat);

  T* data();
  const T* data() const;

  constexpr Mat3x4 operator*(const Mat3x4& _mat) const;
  constexpr Mat3x4 operator+(const Mat3x4& _mat) const;

  constexpr Mat3x4 operator*(T _scalar) const;
  constexpr Mat3x4 operator+(T _scalar) const;

  constexpr Mat3x4& operator*=(const Mat3x4& _mat);
  constexpr Mat3x4& operator+=(const Mat3x4& _mat);

  constexpr Mat3x4& operator*=(T _scalar);
  constexpr Mat3x4& operator+=(T _scalar);

  Vec x, y, z;
};

using Mat3x4f = Mat3x4<Float32>;

template<typename T>
inline constexpr Mat3x4<T>::Mat3x4(const Vec& _x, const Vec& _y, const Vec& _z)
  : x{_x}
  , y{_y}
  , z{_z}
{
}

template<typename T>
inline T* Mat3x4<T>::data() {
  return x.data();
}

template<typename T>
inline const T* Mat3x4<T>::data() const {
  return x.data();
}

template<typename T>
inline Mat3x4<T> Mat3x4<T>::invert(const Mat3x4& _mat) {
  Vec3<T> inverse_rotation_x{_mat.x.x, _mat.y.x, _mat.z.x};
  Vec3<T> inverse_rotation_y{_mat.x.y, _mat.y.y, _mat.z.y};
  Vec3<T> inverse_rotation_z{_mat.x.z, _mat.y.z, _mat.z.z};

  inverse_rotation_x /= dot(inverse_rotation_x, inverse_rotation_x);
  inverse_rotation_y /= dot(inverse_rotation_y, inverse_rotation_y);
  inverse_rotation_z /= dot(inverse_rotation_z, inverse_rotation_z);

  const Vec3<T> translate{_mat.x.w, _mat.y.w, _mat.z.w};

  return {{inverse_rotation_x.x, inverse_rotation_x.y, inverse_rotation_x.z, -dot(inverse_rotation_x, translate)},
          {inverse_rotation_y.x, inverse_rotation_y.y, inverse_rotation_y.z, -dot(inverse_rotation_y, translate)},
          {inverse_rotation_z.x, inverse_rotation_z.y, inverse_rotation_z.z, -dot(inverse_rotation_z, translate)}};
}

template<typename T>
inline constexpr Mat3x4<T> Mat3x4<T>::operator*(const Mat3x4& _mat) const {
  return {(_mat.x*x.x + _mat.y*x.y + _mat.z*x.z) + Vec{0, 0, 0, x.w},
          (_mat.x*y.x + _mat.y*y.y + _mat.z*y.z) + Vec{0, 0, 0, y.w},
          (_mat.x*z.x + _mat.y*z.y + _mat.z*z.z) + Vec{0, 0, 0, z.w}};
}

template<typename T>
inline constexpr Mat3x4<T> Mat3x4<T>::operator+(const Mat3x4& _mat) const {
  return {x + _mat.x, y + _mat.y, z + _mat.z};
}

template<typename T>
inline constexpr Mat3x4<T> Mat3x4<T>::operator*(T _scalar) const {
  return {x * _scalar, y * _scalar, z * _scalar};
}

template<typename T>
inline constexpr Mat3x4<T> Mat3x4<T>::operator+(T _scalar) const {
  return {x + _scalar, y + _scalar, z + _scalar};
}

template<typename T>
inline constexpr Mat3x4<T>& Mat3x4<T>::operator*=(const Mat3x4& _mat) {
  return *this = *this * _mat;
}

template<typename T>
inline constexpr Mat3x4<T>& Mat3x4<T>::operator+=(const Mat3x4& _mat) {
  return *this = *this + _mat;
}

template<typename T>
inline constexpr Mat3x4<T>& Mat3x4<T>::operator*=(T _scalar) {
  return *this = *this * _scalar;
}

template<typename T>
inline constexpr Mat3x4<T>& Mat3x4<T>::operator+=(T _scalar) {
  return *this = *this + _scalar;
}

} // namespace rx::math

#endif // RX_MATH_MAT3X4_H
