#ifndef RX_MATH_VEC3_H
#define RX_MATH_VEC3_H
#include "rx/core/types.h" // rx_size
#include "rx/core/format.h" // format
#include "rx/core/hash.h" // hash, hash_combine
#include "rx/core/assert.h" // RX_ASSERT

#include "rx/core/math/sqrt.h"
#include "rx/core/math/sign.h"
#include "rx/core/math/abs.h"

#include "rx/core/algorithm/min.h"
#include "rx/core/algorithm/max.h"

namespace Rx::Math {

template<typename T>
struct Vec3 {
  constexpr Vec3();
  constexpr Vec3(T _x, T _y, T _z);

  T& operator[](Size _i);
  const T& operator[](Size _i) const;

  bool is_any(T _value) const;
  bool is_all(T _value) const;

  T area() const;
  T sum() const;

  T max_element() const;
  T min_element() const;

  template<typename F>
  Vec3<T> map(F&& _fn) const;

  T* data();
  const T* data() const;

  template<typename T2>
  constexpr Vec3<T2> cast() const;

  void operator+=(const Vec3<T>& _v);
  void operator-=(const Vec3<T>& _v);
  void operator*=(T _scalar);
  void operator/=(T _scalar);

  // TODO: check
  void operator*=(const Vec3<T>& _v);

  union {
    struct { T x, y, z; };
    struct { T w, h, d; };
    struct { T r, g, b; };
    struct { T s, t, p; };
    T array[3];
  };
};

using Vec3f = Vec3<Float32>;
using Vec3i = Vec3<Sint32>;
using Vec3z = Vec3<Size>;

template<typename T>
inline constexpr Vec3<T>::Vec3()
  : x{0}
  , y{0}
  , z{0}
{
}

template<typename T>
inline constexpr Vec3<T>::Vec3(T _x, T _y, T _z)
  : x{_x}
  , y{_y}
  , z{_z}
{
}

template<typename T>
inline T& Vec3<T>::operator[](Size _i) {
  RX_ASSERT(_i < 3, "out of bounds");
  return array[_i];
}

template<typename T>
inline const T& Vec3<T>::operator[](Size _i) const {
  RX_ASSERT(_i < 3, "out of bounds");
  return array[_i];
}

template<typename T>
inline bool Vec3<T>::is_any(T _value) const {
  return x == _value || y == _value || z == _value;
}

template<typename T>
inline bool Vec3<T>::is_all(T _value) const {
  return x == _value && y == _value && z == _value;
}

template<typename T>
inline T Vec3<T>::area() const {
  return x * y * z;
}

template<typename T>
inline T Vec3<T>::sum() const {
  return x + y + z;
}

template<typename T>
inline T Vec3<T>::max_element() const {
  return Algorithm::max(x, y, z);
}

template<typename T>
inline T Vec3<T>::min_element() const {
  return Algorithm::min(x, y, z);
}

template<typename T>
template<typename F>
inline Vec3<T> Vec3<T>::map(F&& _fn) const {
  return { _fn(x), _fn(y), _fn(z) };
}

template<typename T>
inline T* Vec3<T>::data() {
  return array;
}

template<typename T>
inline const T* Vec3<T>::data() const {
  return array;
}

template<typename T>
template<typename T2>
inline constexpr Vec3<T2> Vec3<T>::cast() const {
  return {static_cast<T2>(x), static_cast<T2>(y), static_cast<T2>(z)};
}

template<typename T>
inline void Vec3<T>::operator+=(const Vec3<T>& _v) {
  x += _v.x;
  y += _v.y;
  z += _v.z;
}

template<typename T>
inline void Vec3<T>::operator-=(const Vec3<T>& _v) {
  x -= _v.x;
  y -= _v.y;
  z -= _v.z;
}

template<typename T>
inline void Vec3<T>::operator*=(T _scalar) {
  x *= _scalar;
  y *= _scalar;
  z *= _scalar;
}

template<typename T>
inline void Vec3<T>::operator/=(T _scalar) {
  x /= _scalar;
  y /= _scalar;
  z /= _scalar;
}

// TODO: check
template<typename T>
inline void Vec3<T>::operator*=(const Vec3<T>& _v) {
  x *= _v.x;
  y *= _v.y;
  z *= _v.z;
}

template<typename T>
inline constexpr bool operator==(const Vec3<T>& _lhs, const Vec3<T>& _rhs) {
  return _lhs.x == _rhs.x && _lhs.y == _rhs.y && _lhs.z == _rhs.z;
}

template<typename T>
inline constexpr bool operator!=(const Vec3<T>& _lhs, const Vec3<T>& _rhs) {
  return _lhs.x != _rhs.x || _lhs.y != _rhs.y || _lhs.z != _rhs.z;
}

template<typename T>
inline constexpr Vec3<T> operator-(const Vec3<T>& _v) {
  return {-_v.x, -_v.y, -_v.z};
}

template<typename T>
inline constexpr Vec3<T> operator+(const Vec3<T>& _lhs, const Vec3<T>& _rhs) {
  return {_lhs.x + _rhs.x, _lhs.y + _rhs.y, _lhs.z + _rhs.z};
}

template<typename T>
inline constexpr Vec3<T> operator-(const Vec3<T>& _lhs, const Vec3<T>& _rhs) {
  return {_lhs.x - _rhs.x, _lhs.y - _rhs.y, _lhs.z - _rhs.z};
}

template<typename T>
inline constexpr Vec3<T> operator*(T _scalar, const Vec3<T>& _v) {
  return {_scalar * _v.x, _scalar * _v.y, _scalar * _v.z};
}

template<typename T>
inline constexpr Vec3<T> operator*(const Vec3<T>& _v, T _scalar) {
  return _scalar * _v;
}

// NOTE: check
template<typename T>
inline constexpr Vec3<T> operator-(const Vec3<T>& _v, T _scalar) {
  return {_v.x - _scalar, _v.y - _scalar, _v.z - _scalar};
}

// NOTE: check
template<typename T>
inline constexpr Vec3<T> operator+(const Vec3<T>& _v, T _scalar) {
  return {_v.x + _scalar, _v.y + _scalar, _v.z + _scalar};
}

// NOTE: check
template<typename T>
inline constexpr Vec3<T> operator/(T _scalar, const Vec3<T>& _v) {
  return {_scalar / _v.x, _scalar / _v.y, _scalar / _v.z};
}

// NOTE: check
template<typename T>
inline constexpr Vec3<T> operator/(const Vec3<T>& _v, T _scalar) {
  return {_v.x / _scalar, _v.y / _scalar, _v.z / _scalar};
}

// NOTE: check
template<typename T>
inline constexpr Vec3<T> operator/(const Vec3<T>& _lhs, const Vec3<T>& _rhs) {
  return {_lhs.x / _rhs.x, _lhs.y / _rhs.y, _lhs.z / _rhs.z};
}

// NOTE: check
template<typename T>
inline constexpr Vec3<T> operator*(const Vec3<T>& _lhs, const Vec3<T>& _rhs) {
  return {_lhs.x * _rhs.x, _lhs.y * _rhs.y, _lhs.z * _rhs.z};
}

template<typename T>
inline constexpr bool operator<(const Vec3<T>& _a, const Vec3<T>& _b) {
  return _a.x < _b.x && _a.y < _b.y && _a.z < _b.z;
}

template<typename T>
inline constexpr bool operator>(const Vec3<T>& _a, const Vec3<T>& _b) {
  return _a.x > _b.x && _a.y > _b.y && _a.z > _b.z;
}

// Functions.
template<typename T>
inline constexpr T dot(const Vec3<T>& _lhs, const Vec3<T>& _rhs) {
  return _lhs.x * _rhs.x + _lhs.y * _rhs.y + _lhs.z * _rhs.z;
}

template<typename T>
inline constexpr Vec3<T> cross(const Vec3<T>& _lhs, const Vec3<T>& _rhs) {
  return {_lhs.y * _rhs.z - _rhs.y * _lhs.z,
          _lhs.z * _rhs.x - _rhs.z * _lhs.x,
          _lhs.x * _rhs.y - _rhs.x * _lhs.y};
}

// Compute the determinant of a matrix whose columns are three given vectors.
template<typename T>
inline constexpr T det(const Vec3<T>& _a, const Vec3<T>& _b, const Vec3<T>& _c) {
  return dot(_a, cross(_b, _c));
}

// Compute minimum vector between two vectors (per-element).
template<typename T>
inline constexpr Vec3<T> min(const Vec3<T>& _a, const Vec3<T>& _b) {
  return {Algorithm::min(_a.x, _b.x), Algorithm::min(_a.y, _b.y), Algorithm::min(_a.z, _b.z)};
}

// Compute maximum vector between two vectors (per-element).
template<typename T>
inline constexpr Vec3<T> max(const Vec3<T>& _a, const Vec3<T>& _b) {
  return {Algorithm::max(_a.x, _b.x), Algorithm::max(_a.y, _b.y), Algorithm::max(_a.z, _b.z)};
}

// Compute absolute vector of a given vectgor (per-element).
template<typename T>
inline Vec3<T> abs(const Vec3<T>& _v) {
  return {abs(_v.x), abs(_v.y), abs(_v.y)};
}

// Only defined for floating point
inline Float32 length_squared(const Vec3f& _v) {
  return dot(_v, _v);
}

inline Float32 length(const Vec3f& _v) {
  return sqrt(length_squared(_v));
}

inline Vec3f normalize(const Vec3f& _v) {
  return (1.0f / length(_v)) * _v;
}

// Compute euclidean distance between two points.
inline Float32 distance(const Vec3f& _a, const Vec3f& _b) {
  return length(_a - _b);
}

// Compute squared distance between two points.
inline Float32 distance_squared(const Vec3f& _a, const Vec3f& _b) {
  return length_squared(_a - _b);
}

// Compute triangle area.
inline Float32 area(const Vec3f& _a, const Vec3f& _b, const Vec3f& _c) {
  return 0.5f * length(cross(_b - _a, _c - _a));
}

// Compute squared triangle area.
inline Float32 squared_area(const Vec3f& _a, const Vec3f& _b, const Vec3f& _c) {
  return 0.25f * length_squared(cross(_b - _a, _c - _a));
}

// Compute tetrahedron volume.
inline Float32 volume(const Vec3f& _a, const Vec3f& _b, const Vec3f& _c,
                     const Vec3f& _d)
{
  const Float32 volume{det(_b - _a, _c - _a, _d - _a)};
  return sign(volume) * (1.0f / 6.0f) * volume;
}

// Compute squared tetrahdron volume.
inline Float32 volume_squared(const Vec3f& _a, const Vec3f& _b, const Vec3f& _c,
                             const Vec3f& _d)
{
  const Float32 result{volume(_a, _b, _c, _d)};
  return result * result;
}

// Find a perpendicular vector to a vector.
inline Vec3f perp(const Vec3f& _v) {
  // Suppose vector a has all equal components and is a unit vector: a = (s, s, s)
  // Then 3*s*s = 1, s = sqrt(1/3) = 0.57735. This means at least one component of
  // a unit vector must be greater than or equal to 0.557735.
  if (abs(_v.x) >= 0.557735f) {
    return {_v.y, -_v.x, 0.0f};
  }

  return {0.0f, _v.z, -_v.x};
}

} // namespace rx::math

namespace Rx {
  template<>
  struct FormatNormalize<Math::Vec3f> {
    char scratch[FormatSize<Float32>::size * 3 + sizeof "{,,  }" - 1];
    const char* operator()(const Math::Vec3f& _value);
  };

  template<>
  struct FormatNormalize<::Rx::Math::Vec3i> {
    char scratch[FormatSize<Sint32>::size * 3 + sizeof "{,,  }" - 1];
    const char* operator()(const Math::Vec3i& _value);
  };

  template<>
  struct Hash<Math::Vec3f> {
    Size operator()(const Math::Vec3f& _value) {
      const auto x{Hash<Float32>{}(_value.x)};
      const auto y{Hash<Float32>{}(_value.y)};
      const auto z{Hash<Float32>{}(_value.z)};
      return hash_combine(hash_combine(x, y), z);
    }
  };

  template<>
  struct Hash<Math::Vec3i> {
    Size operator()(const Math::Vec3i& _value) {
      const auto x{Hash<Sint32>{}(_value.x)};
      const auto y{Hash<Sint32>{}(_value.y)};
      const auto z{Hash<Sint32>{}(_value.z)};
      return hash_combine(hash_combine(x, y), z);
    }
  };
} // namespace rx

#endif // RX_MATH_VEC3_H
