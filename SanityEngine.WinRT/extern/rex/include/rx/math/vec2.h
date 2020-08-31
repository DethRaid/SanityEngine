#ifndef RX_MATH_VEC2_H
#define RX_MATH_VEC2_H
#include "rx/core/types.h" // Size
#include "rx/core/format.h" // format
#include "rx/core/hash.h" // hash, hash_combine
#include "rx/core/assert.h" // RX_ASSERT

#include "rx/core/algorithm/min.h" // algorithm::min
#include "rx/core/algorithm/max.h" // algorithm::max

#include "rx/core/math/sqrt.h" // sqrt

namespace Rx::Math {

template<typename T>
struct Vec2 {
  constexpr Vec2();
  constexpr Vec2(T _x, T _y);

  T& operator[](Size _i);
  const T& operator[](Size _i) const;

  bool is_any(T _value) const;
  bool is_all(T _value) const;

  T area() const;
  T sum() const;

  T max_element() const;
  T min_element() const;

  template<typename F>
  Vec2<T> map(F&& _fn) const;

  T* data();
  const T* data() const;

  template<typename T2>
  constexpr Vec2<T2> cast() const;

  void operator+=(const Vec2<T>& _v);
  void operator-=(const Vec2<T>& _v);
  void operator*=(T _scalar);
  void operator/=(T _scalar);

  union {
    struct { T x, y; };
    struct { T w, h; };
    struct { T r, g; };
    struct { T u, v; };
    struct { T s, t; };
    T array[2];
  };
};

using Vec2f = Vec2<Float32>;
using Vec2i = Vec2<Sint32>;
using Vec2z = Vec2<Size>;

template<typename T>
inline constexpr Vec2<T>::Vec2()
  : x{0}
  , y{0}
{
}

template<typename T>
inline constexpr Vec2<T>::Vec2(T _x, T _y)
  : x{_x}
  , y{_y}
{
}

template<typename T>
inline T& Vec2<T>::operator[](Size _i) {
  RX_ASSERT(_i < 2, "out of bounds");
  return array[_i];
}

template<typename T>
inline const T& Vec2<T>::operator[](Size _i) const {
  RX_ASSERT(_i < 2, "out of bounds");
  return array[_i];
}

template<typename T>
inline bool Vec2<T>::is_any(T _value) const {
  return x == _value || y == _value;
}

template<typename T>
inline bool Vec2<T>::is_all(T _value) const {
  return x == _value && y == _value;
}

template<typename T>
inline T Vec2<T>::area() const {
  return x * y;
}

template<typename T>
inline T Vec2<T>::sum() const {
  return x + y;
}

template<typename T>
inline T Vec2<T>::max_element() const {
  return Algorithm::max(x, y);
}

template<typename T>
inline T Vec2<T>::min_element() const {
  return Algorithm::min(x, y);
}

template<typename T>
template<typename F>
inline Vec2<T> Vec2<T>::map(F&& _fn) const {
  return { _fn(x), _fn(y) };
}

template<typename T>
inline T* Vec2<T>::data() {
  return array;
}

template<typename T>
inline const T* Vec2<T>::data() const {
  return array;
}

template<typename T>
template<typename T2>
inline constexpr Vec2<T2> Vec2<T>::cast() const {
  return {static_cast<T2>(x), static_cast<T2>(y)};
}

template<typename T>
inline void Vec2<T>::operator+=(const Vec2<T>& _v) {
  x += _v.x;
  y += _v.y;
}

template<typename T>
inline void Vec2<T>::operator-=(const Vec2<T>& _v) {
  x -= _v.x;
  y -= _v.y;
}

template<typename T>
inline void Vec2<T>::operator*=(T _scalar) {
  x *= _scalar;
  y *= _scalar;
}

template<typename T>
inline void Vec2<T>::operator/=(T _scalar) {
  x /= _scalar;
  y /= _scalar;
}

template<typename T>
inline constexpr bool operator==(const Vec2<T>& _lhs, const Vec2<T>& _rhs) {
  return _lhs.x == _rhs.x && _lhs.y == _rhs.y;
}

template<typename T>
inline constexpr bool operator!=(const Vec2<T>& _lhs, const Vec2<T>& _rhs) {
  return _lhs.x != _rhs.x || _lhs.y != _rhs.y;
}

template<typename T>
inline constexpr Vec2<T> operator-(const Vec2<T>& _v) {
  return {-_v.x, -_v.y};
}

template<typename T>
inline constexpr Vec2<T> operator+(const Vec2<T>& _a, const Vec2<T>& _b) {
  return {_a.x + _b.x, _a.y + _b.y};
}

template<typename T>
inline constexpr Vec2<T> operator-(const Vec2<T>& _a, const Vec2<T>& _b) {
  return {_a.x - _b.x, _a.y - _b.y};
}

template<typename T>
inline constexpr Vec2<T> operator*(T _scalar, const Vec2<T>& _v) {
  return {_scalar * _v.x, _scalar * _v.y};
}

template<typename T>
inline constexpr Vec2<T> operator*(const Vec2<T>& _v, T _scalar) {
  return _scalar * _v;
}

// NOTE: check
template<typename T>
inline constexpr Vec2<T> operator-(const Vec2<T>& _v, T _scalar) {
  return {_v.x - _scalar, _v.y - _scalar};
}

// NOTE: check
template<typename T>
inline constexpr Vec2<T> operator+(const Vec2<T>& _v, T _scalar) {
  return {_v.x + _scalar, _v.y + _scalar};
}

// NOTE: check
template<typename T>
inline constexpr Vec2<T> operator/(T _scalar, const Vec2<T>& _v) {
  return {_scalar / _v.x, _scalar / _v.y};
}

// NOTE: check
template<typename T>
inline constexpr Vec2<T> operator/(const Vec2<T>& _v, T _scalar) {
  return {_v.x / _scalar, _v.y / _scalar};
}

// NOTE: check
template<typename T>
inline constexpr Vec2<T> operator/(const Vec2<T>& _a, const Vec2<T>& _b) {
  return {_a.x / _b.x, _a.y / _b.y};
}

// NOTE: check
template<typename T>
inline constexpr Vec2<T> operator*(const Vec2<T>& _a, const Vec2<T>& _b) {
  return {_a.x * _b.x, _a.y * _b.y};
}

template<typename T>
inline constexpr bool operator<(const Vec2<T>& _a, const Vec2<T>& _b) {
  return _a.x < _b.x && _a.y < _b.y;
}

template<typename T>
inline constexpr bool operator>(const Vec2<T>& _a, const Vec2<T>& _b) {
  return _a.x > _b.x && _a.y > _b.y;
}

template<typename T>
inline constexpr T dot(const Vec2<T>& _lhs, const Vec2<T>& _rhs) {
  return _lhs.x * _rhs.x + _lhs.y * _rhs.y;
}

// Only defined for floating point
inline Float32 length(const Vec2f& _value) {
  return sqrt(dot(_value, _value));
}

template<typename T>
inline Float32 distance(const Vec2f& _a, const Vec2f& _b) {
  return length(_a - _b);
}

inline Vec2f normalize(const Vec2f& _v) {
  return (1.0f / length(_v)) * _v;
}

} // namespace rx::math

namespace Rx {
  template<>
  struct FormatNormalize<Math::Vec2f> {
    char scratch[FormatSize<Float32>::size * 2 + sizeof "{, }" - 1];
    const char* operator()(const Math::Vec2f& _value);
  };

  template<>
  struct FormatNormalize<Math::Vec2i> {
    char scratch[FormatSize<Sint32>::size * 2 + sizeof "{, }" - 1];
    const char* operator()(const Math::Vec2i& _value);
  };

  template<>
  struct Hash<Math::Vec2f> {
    Size operator()(const Math::Vec2f& _value) {
      const auto x{Hash<Float32>{}(_value.x)};
      const auto y{Hash<Float32>{}(_value.y)};
      return hash_combine(x, y);
    }
  };

  template<>
  struct Hash<Math::Vec2i> {
    Size operator()(const Math::Vec2i& _value) {
      const auto x{Hash<Sint32>{}(_value.x)};
      const auto y{Hash<Sint32>{}(_value.y)};
      return hash_combine(x, y);
    }
  };

  template<>
  struct Hash<Math::Vec2z> {
    Size operator()(const Math::Vec2z& _value) {
      const auto x{Hash<Size>{}(_value.x)};
      const auto y{Hash<Size>{}(_value.y)};
      return hash_combine(x, y);
    }
  };
} // namespace rx

#endif // RX_MATH_VEC2_H
