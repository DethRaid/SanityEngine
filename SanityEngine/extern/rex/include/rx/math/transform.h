#ifndef RX_MATH_TRANSFORM_H
#define RX_MATH_TRANSFORM_H

#include "rx/math/mat3x3.h" // mat3x3, vec3f
#include "rx/math/mat4x4.h" // mat4x4

namespace Rx::Math {

struct Transform {
  constexpr Transform();
  constexpr Transform(Transform* parent);

  Mat4x4f as_mat4() const;
  Mat3x3f as_mat3() const;

  Mat4x4f as_local_mat4() const;
  Mat3x3f as_local_mat3() const;

  Vec3f scale;
  Vec3f rotate;
  Vec3f translate;

  const Transform* parent;
};

inline constexpr Transform::Transform()
  : Transform{nullptr}
{
}

inline constexpr Transform::Transform(Transform* _parent)
  : scale{1.0f, 1.0f, 1.0f}
  , rotate{}
  , translate{}
  , parent{_parent}
{
}

inline Mat4x4f Transform::as_mat4() const {
  const auto& local = as_local_mat4();
  return parent ? local * parent->as_mat4() : local;
}

inline Mat4x4f Transform::as_local_mat4() const {
  return Mat4x4f::scale(scale) * Mat4x4f::rotate(rotate) * Mat4x4f::translate(translate);
}

inline Mat3x3f Transform::as_mat3() const {
  const auto& local = as_local_mat3();
  return parent ? local * parent->as_mat3() : local;
}

inline Mat3x3f Transform::as_local_mat3() const {
  return Mat3x3f::scale(scale) * Mat3x3f::rotate(rotate) * Mat3x3f::translate(translate);
}

} // namespace rx::math

#endif // RX_MATH_TRANSFORM_H
