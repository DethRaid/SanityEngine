#ifndef RX_MATH_VEC4_H
#define RX_MATH_VEC4_H
#include "rx/core/types.h" // Size
#include "rx/core/format.h" // format
#include "rx/core/hash.h" // hash, hash_combine
#include "rx/core/assert.h" // RX_ASSERT

#include "rx/core/math/sqrt.h"

#include "rx/core/algorithm/min.h" // algorithm::min
#include "rx/core/algorithm/max.h" // algorithm::max

namespace Rx::Math {

template<typename T>
struct Vec4 {
  constexpr Vec4();
  constexpr Vec4(T _x, T _y, T _z, T _w);

  T& operator[](Size _i);
  const T& operator[](Size _i) const;

  bool is_any(T _value) const;
  bool is_all(T _value) const;

  T area() const;
  T sum() const;

  T max_element() const;
  T min_element() const;

  template<typename F>
  Vec4<T> map(F&& _fn) const;

  T* data();
  const T* data() const;

  template<typename T2>
  Vec4<T2> cast() const;

  union {
    struct { T x, y, z, w; };
    struct { T r, g, b, a; };
    T array[4];
  };
};

using Vec4b = Vec4<Byte>;
using Vec4f = Vec4<Float32>;
using Vec4i = Vec4<Sint32>;
using Vec4z = Vec4<Size>;

template<typename T>
inline constexpr Vec4<T>::Vec4()
  : x{0}
  , y{0}
  , z{0}
  , w{0}
{
}

template<typename T>
inline constexpr Vec4<T>::Vec4(T _x, T _y, T _z, T _w)
  : x{_x}
  , y{_y}
  , z{_z}
  , w{_w}
{
}

template<typename T>
inline T& Vec4<T>::operator[](Size _i) {
  RX_ASSERT(_i < 4, "out of bounds");
  return array[_i];
}

template<typename T>
inline const T& Vec4<T>::operator[](Size _i) const {
  RX_ASSERT(_i < 4, "out of bounds");
  return array[_i];
}

template<typename T>
inline bool Vec4<T>::is_any(T _value) const {
  return x == _value || y == _value || z == _value || w == _value;
}

template<typename T>
inline bool Vec4<T>::is_all(T _value) const {
  return x == _value && y == _value && z == _value && w == _value;
}

template<typename T>
inline T Vec4<T>::area() const {
  return x * y * z * w;
}

template<typename T>
inline T Vec4<T>::sum() const {
  return x + y + z + w;
}

template<typename T>
inline T Vec4<T>::max_element() const {
  return Algorithm::max(x, y, z, w);
}

template<typename T>
inline T Vec4<T>::min_element() const {
  return Algorithm::min(x, y, z, w);
}

template<typename T>
template<typename F>
inline Vec4<T> Vec4<T>::map(F&& _fn) const {
  return { _fn(x), _fn(y), _fn(z), _fn(w) };
}

template<typename T>
inline T* Vec4<T>::data() {
  return array;
}

template<typename T>
inline const T* Vec4<T>::data() const {
  return array;
}

template<typename T>
template<typename T2>
inline Vec4<T2> Vec4<T>::cast() const {
  return {static_cast<T2>(x), static_cast<T2>(y), static_cast<T2>(z), static_cast<T2>(w)};
}

template<typename T>
inline constexpr bool operator==(const Vec4<T>& _lhs, const Vec4<T>& _rhs) {
  return _lhs.x == _rhs.x && _lhs.y == _rhs.y && _lhs.z == _rhs.z && _lhs.w == _rhs.w;
}

template<typename T>
inline constexpr bool operator!=(const Vec4<T>& _lhs, const Vec4<T>& _rhs) {
  return _lhs.x != _rhs.x || _lhs.y != _rhs.y || _lhs.z != _rhs.z || _lhs.w != _rhs.w;
}

template<typename T>
inline constexpr Vec4<T> operator+(const Vec4<T>& _lhs, const Vec4<T>& _rhs) {
  return {_lhs.x + _rhs.x, _lhs.y + _rhs.y, _lhs.z + _rhs.z, _lhs.w + _rhs.w};
}

template<typename T>
inline constexpr Vec4<T> operator-(const Vec4<T>& _lhs, const Vec4<T>& _rhs) {
  return {_lhs.x - _rhs.x, _lhs.y - _rhs.y, _lhs.z - _rhs.z, _lhs.w - _rhs.w};
}

template<typename T>
inline constexpr Vec4<T> operator*(T _scalar, const Vec4<T>& _v) {
  return {_scalar * _v.x, _scalar * _v.y, _scalar * _v.z, _scalar * _v.w};
}

template<typename T>
inline constexpr Vec4<T> operator*(const Vec4<T>& _v, T _scalar) {
  return _scalar * _v;
}

template<typename T>
inline constexpr Vec4<T> operator<(const Vec4<T>& _a, const Vec4<T>& _b) {
  return _a.x < _b.x && _a.y < _b.y && _a.z < _b.z && _a.w < _b.w;
}

template<typename T>
inline constexpr Vec4<T> operator>(const Vec4<T>& _a, const Vec4<T>& _b) {
  return _a.x > _b.x && _a.y > _b.y && _a.z > _b.z && _a.w > _b.w;
}

// TODO: check
// template<typename T>
// inline constexpr T operator*(const vec4<T>& _lhs, const vec4<T>& _rhs) {
//   return _lhs.x * _rhs.x + _lhs.y * _rhs.y + _lhs.z * _rhs.z + _lhs.w * _rhs.w;
// }

// Functions.
template<typename T>
inline constexpr T dot(const Vec4<T>& _lhs, const Vec4<T>& _rhs) {
  return _lhs.x * _rhs.x + _lhs.y * _rhs.y + _lhs.z * _rhs.z + _lhs.w * _rhs.w;
}

// Only defined for floating point
inline Float32 length_squared(const Vec4f& _v) {
  return dot(_v, _v);
}

inline Float32 length(const Vec4f& _v) {
  return sqrt(length_squared(_v));
}

inline Vec4f normalize(const Vec4f& _v) {
  return (1.0f / length(_v)) * _v;
}

} // namespace rx::math

namespace Rx {
  template<>
  struct FormatNormalize<Math::Vec4f> {
    char scratch[FormatSize<Float32>::size * 4 + sizeof "{,,,   }" - 1];
    const char* operator()(const Math::Vec4f& _value);
  };

  template<>
  struct FormatNormalize<Math::Vec4i> {
    char scratch[FormatSize<Sint32>::size * 4 + sizeof "{,,,   }" - 1];
    const char* operator()(const Math::Vec4i& _value);
  };

  template<>
  struct Hash<Math::Vec4f> {
    Size operator()(const Math::Vec4f& _value) {
      const auto x{Hash<Float32>{}(_value.x)};
      const auto y{Hash<Float32>{}(_value.y)};
      const auto z{Hash<Float32>{}(_value.z)};
      const auto w{Hash<Float32>{}(_value.w)};
      return hash_combine(hash_combine(x, y), hash_combine(z, w));
    }
  };

  template<>
  struct Hash<Math::Vec4i> {
    Size operator()(const Math::Vec4i& _value) {
      const auto x{Hash<Sint32>{}(_value.x)};
      const auto y{Hash<Sint32>{}(_value.y)};
      const auto z{Hash<Sint32>{}(_value.z)};
      const auto w{Hash<Sint32>{}(_value.w)};
      return hash_combine(hash_combine(x, y), hash_combine(z, w));
    }
  };
} // namespace rx

#endif // RX_MATH_VEC4_H
