#ifndef RX_MATH_MAT4X4_H
#define RX_MATH_MAT4X4_H
#include "rx/math/vec3.h" // vec3
#include "rx/math/vec4.h" // vec4
#include "rx/math/trig.h" // deg_to_rad
#include "rx/math/compare.h"

#include "rx/core/math/sin.h" // sin
#include "rx/core/math/cos.h" // cos
#include "rx/core/math/tan.h" // tan

namespace Rx::Math {

template<typename T>
struct Mat4x4 {
  using Vec = Vec4<T>;

  constexpr Mat4x4();
  constexpr Mat4x4(const Vec& _x, const Vec& _y, const Vec& _z, const Vec& _w);

  T* data();
  const T* data() const;

  static constexpr Mat4x4 scale(const Vec3<T>& _scale);
  static constexpr Mat4x4 rotate(const Vec3<T>& _rotate);
  static constexpr Mat4x4 translate(const Vec3<T>& _translate);
  static constexpr Mat4x4 transpose(const Mat4x4& _mat);
  static constexpr Mat4x4 invert(const Mat4x4& _mat);
  static constexpr Mat4x4 perspective(T _fov, const range<T>& _planes, T _aspect);

  static constexpr Vec3<T> transform_point(const Vec3<T>& _point, const Mat4x4& _mat);
  static constexpr Vec3<T> transform_vector(const Vec3<T>& _vector, const Mat4x4& _mat);

  constexpr Mat4x4 operator*(const Mat4x4& _mat) const;
  constexpr Mat4x4 operator+(const Mat4x4& _mat) const;

  constexpr Mat4x4 operator*(T _scalar) const;
  constexpr Mat4x4 operator+(T _scalar) const;

  constexpr Mat4x4& operator*=(const Mat4x4& _mat);
  constexpr Mat4x4& operator+=(const Mat4x4& _mat);

  constexpr Mat4x4& operator*=(T _scalar);
  constexpr Mat4x4& operator+=(T _scalar);

  template<typename U>
  friend constexpr bool operator==(const Mat4x4<U>& _lhs, const Mat4x4<U>& _rhs);
  template<typename U>
  friend constexpr bool operator!=(const Mat4x4<U>& _lhs, const Mat4x4<U>& _rhs);

  Vec x, y, z, w;

private:
  static constexpr T det2x2(T a, T b, T c, T d);
  static constexpr T det3x3(T a1, T a2, T a3, T b1, T b2, T b3, T c1, T c2, T c3);

  static constexpr Vec3<T> reduce_rotation_angles(const Vec3<T>& _rotate);
};

using Mat4x4f = Mat4x4<float>;

template<typename T>
constexpr Mat4x4<T>::Mat4x4()
  : x{1, 0, 0, 0}
  , y{0, 1, 0, 0}
  , z{0, 0, 1, 0}
  , w{0, 0, 0, 1}
{
}

template<typename T>
constexpr Mat4x4<T>::Mat4x4(const Vec& x, const Vec& y, const Vec& z, const Vec& w)
  : x{x}
  , y{y}
  , z{z}
  , w{w}
{
}

template<typename T>
inline T* Mat4x4<T>::data() {
  return x.data();
}

template<typename T>
inline const T* Mat4x4<T>::data() const {
  return x.data();
}

template<typename T>
inline constexpr Mat4x4<T> Mat4x4<T>::scale(const Vec3<T>& _scale) {
  return {{_scale.x, 0,        0,        0},
          {0,        _scale.y, 0,        0},
          {0,        0,        _scale.z, 0},
          {0,        0,        0,        1}};
}

template<typename T>
inline constexpr Mat4x4<T> Mat4x4<T>::rotate(const Vec3<T>& _rotate) {
  const auto reduce{reduce_rotation_angles(_rotate)};
  const auto sx{sin(deg_to_rad(-reduce.x))};
  const auto cx{cos(deg_to_rad(-reduce.x))};
  const auto sy{sin(deg_to_rad(-reduce.y))};
  const auto cy{cos(deg_to_rad(-reduce.y))};
  const auto sz{sin(deg_to_rad(-reduce.z))};
  const auto cz{cos(deg_to_rad(-reduce.z))};
  return {{ cy*cz,              cy*-sz,              sy,    0},
          {-sx*-sy*cz + cx*sz, -sx*-sy*-sz + cx*cz, -sx*cy, 0},
          { cx*-sy*cz + sx*sz,  cx*-sy*-sz + sx*cz,  cx*cy, 0},
          { 0,                  0,                   0,     1}};
}

template<typename T>
inline constexpr Mat4x4<T> Mat4x4<T>::translate(const Vec3<T>& _translate) {
  return {{1,            0,            0,            0},
          {0,            1,            0,            0},
          {0,            0,            1,            0},
          {_translate.x, _translate.y, _translate.z, 1}};

}

template<typename T>
inline constexpr Mat4x4<T> Mat4x4<T>::transpose(const Mat4x4& _mat) {
  return {{_mat.x.x, _mat.y.x, _mat.z.x, _mat.w.x},
          {_mat.x.y, _mat.y.y, _mat.z.y, _mat.w.y},
          {_mat.x.z, _mat.y.z, _mat.z.z, _mat.w.z}};
}

template<typename T>
inline constexpr Mat4x4<T> Mat4x4<T>::invert(const Mat4x4& _mat) {
  const auto a1{_mat.x.x}, a2{_mat.x.y}, a3{_mat.x.z}, a4{_mat.x.w};
  const auto b1{_mat.y.x}, b2{_mat.y.y}, b3{_mat.y.z}, b4{_mat.y.w};
  const auto c1{_mat.z.x}, c2{_mat.z.y}, c3{_mat.z.z}, c4{_mat.z.w};
  const auto d1{_mat.w.x}, d2{_mat.w.y}, d3{_mat.w.z}, d4{_mat.w.w};

  const auto det1{ det3x3(b2, b3, b4, c2, c3, c4, d2, d3, d4)};
  const auto det2{-det3x3(a2, a3, a4, c2, c3, c4, d2, d3, d4)};
  const auto det3{ det3x3(a2, a3, a4, b2, b3, b4, d2, d3, d4)};
  const auto det4{-det3x3(a2, a3, a4, b2, b3, b4, c2, c3, c4)};

  const auto det{a1*det1 + b1*det2 + c1*det3 + d1*det4};

  if (epsilon_compare(det, 0.0f)) {
    return {};
  }

  const auto invdet{T{1} / det};

  return {
    {
      det1 * invdet,
      det2 * invdet,
      det3 * invdet,
      det4 * invdet
    },{
      -det3x3(b1, b3, b4, c1, c3, c4, d1, d3, d4) * invdet,
       det3x3(a1, a3, a4, c1, c3, c4, d1, d3, d4) * invdet,
      -det3x3(a1, a3, a4, b1, b3, b4, d1, d3, d4) * invdet,
       det3x3(a1, a3, a4, b1, b3, b4, c1, c3, c4) * invdet,
    },{
       det3x3(b1, b2, b4, c1, c2, c4, d1, d2, d4) * invdet,
      -det3x3(a1, a2, a4, c1, c2, c4, d1, d2, d4) * invdet,
       det3x3(a1, a2, a4, b1, b2, b4, d1, d2, d4) * invdet,
      -det3x3(a1, a2, a4, b1, b2, b4, c1, c2, c4) * invdet
    },{
      -det3x3(b1, b2, b3, c1, c2, c3, d1, d2, d3) * invdet,
       det3x3(a1, a2, a3, c1, c2, c3, d1, d2, d3) * invdet,
      -det3x3(a1, a2, a3, b1, b2, b3, d1, d2, d3) * invdet,
       det3x3(a1, a2, a3, b1, b2, b3, c1, c2, c3) * invdet
    }
  };
}

template<typename T>
inline constexpr Mat4x4<T> Mat4x4<T>::perspective(T _fov, const range<T>& _planes, T _aspect) {
  const T range{_planes.min - _planes.max};
  const T half{tan(deg_to_rad(_fov*T{.5}))};
  if (_aspect < 1) {
    return {{1 / half,             0,                    0,                                      0},
            {0,                    1 / (half / _aspect), 0,                                      0},
            {0,                    0,                    -(_planes.min + _planes.max) / range,   1},
            {0,                    0,                    2 * _planes.max * _planes.min / range,  0}};
  } else {
    return {{1 / (half * _aspect), 0,                    0,                                      0},
            {0,                    1 / half,             0,                                      0},
            {0,                    0,                    -(_planes.min + _planes.max) / range,   1},
            {0,                    0,                    2 * _planes.max * _planes.min / range,  0}};
  }
}

template<typename T>
inline constexpr Vec3<T> Mat4x4<T>::transform_point(const Vec3<T>& _point, const Mat4x4<T>& _mat) {
  const Vec3<T>& w{_mat.w.x, _mat.w.y, _mat.w.z};
  return transform_vector(_point, _mat) + w;
}

template<typename T>
inline constexpr Vec3<T> Mat4x4<T>::transform_vector(const Vec3<T>& _vector, const Mat4x4<T>& _mat) {
  const Vec3<T>& x{_mat.x.x, _mat.x.y, _mat.x.z};
  const Vec3<T>& y{_mat.y.x, _mat.y.y, _mat.y.z};
  const Vec3<T>& z{_mat.z.x, _mat.z.y, _mat.z.z};
  return x * _vector.x + y * _vector.y + z * _vector.z;
}

template<typename T>
inline constexpr Mat4x4<T> Mat4x4<T>::operator*(const Mat4x4& _mat) const {
  return {_mat.x*x.x + _mat.y*x.y + _mat.z*x.z + _mat.w*x.w,
          _mat.x*y.x + _mat.y*y.y + _mat.z*y.z + _mat.w*y.w,
          _mat.x*z.x + _mat.y*z.y + _mat.z*z.z + _mat.w*z.w,
          _mat.x*w.x + _mat.y*w.y + _mat.z*w.z + _mat.w*w.w};
}

template<typename T>
inline constexpr Mat4x4<T> Mat4x4<T>::operator+(const Mat4x4& _mat) const {
  return {x + _mat.x, y + _mat.y, z + _mat.z, w + _mat.w};
}

template<typename T>
inline constexpr Mat4x4<T> Mat4x4<T>::operator*(T _scalar) const {
  return {x * _scalar, y * _scalar, z * _scalar, w * _scalar};
}

template<typename T>
inline constexpr Mat4x4<T> Mat4x4<T>::operator+(T _scalar) const {
  return {x + _scalar, y + _scalar, z + _scalar, w + _scalar};
}

template<typename T>
inline constexpr Mat4x4<T>& Mat4x4<T>::operator*=(const Mat4x4& _mat) {
  return *this = *this * _mat;
}

template<typename T>
inline constexpr Mat4x4<T>& Mat4x4<T>::operator+=(const Mat4x4& _mat) {
  return *this = *this + _mat;
}

template<typename T>
inline constexpr Mat4x4<T>& Mat4x4<T>::operator*=(T _scalar) {
  return *this = *this * _scalar;
}

template<typename T>
inline constexpr Mat4x4<T>& Mat4x4<T>::operator+=(T _scalar) {
  return *this = *this + _scalar;
}

template<typename U>
constexpr bool operator==(const Mat4x4<U>& _lhs, const Mat4x4<U>& _rhs) {
  return _lhs.x == _rhs.x && _lhs.y == _rhs.y && _lhs.z == _rhs.z && _lhs.w == _rhs.w;
}

template<typename U>
constexpr bool operator!=(const Mat4x4<U>& _lhs, const Mat4x4<U>& _rhs) {
  return _lhs.x != _rhs.x || _lhs.y != _rhs.y || _lhs.z != _rhs.z || _lhs.w != _rhs.w;
}

template<typename T>
inline constexpr T Mat4x4<T>::det2x2(T a, T b, T c, T d) {
  return a*d - b*c;
}

template<typename T>
inline constexpr T Mat4x4<T>::det3x3(T a1, T a2, T a3, T b1, T b2, T b3, T c1, T c2, T c3) {
  return a1 * det2x2(b2, b3, c2, c3) - b1 * det2x2(a2, a3, c2, c3) + c1 * det2x2(a2, a3, b2, b3);
}

template<typename T>
inline constexpr Vec3<T> Mat4x4<T>::reduce_rotation_angles(const Vec3<T>& _rotate) {
  return _rotate.map([](T _angle) {
    while (_angle >  180) {
      _angle -= 360;
    }

    while (_angle < -180) {
      _angle += 360;
    }

    return _angle;
  });
}

} // namespace rx::math

namespace Rx {
  template<>
  struct Hash<Math::Mat4x4f> {
    Size operator()(const Math::Mat4x4f& _value) {
      const auto x{Hash<Math::Vec4f>{}(_value.x)};
      const auto y{Hash<Math::Vec4f>{}(_value.y)};
      const auto z{Hash<Math::Vec4f>{}(_value.z)};
      const auto w{Hash<Math::Vec4f>{}(_value.w)};
      return hash_combine(hash_combine(x, hash_combine(y, z)), w);
    }
  };
}

#endif // RX_MATH_MAT4X4_H
