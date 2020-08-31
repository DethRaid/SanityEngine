#ifndef RX_MATH_CAMERA_H
#define RX_MATH_CAMERA_H
#include "rx/math/transform.h"

namespace Rx::Math {

struct Camera
  : Transform
{
  constexpr Camera(Transform* _parent = nullptr);

  Mat4x4f view() const;
  Mat4x4f projection;
};

inline constexpr Camera::Camera(Transform* _parent)
  : Transform{_parent}
{
}

inline Mat4x4f Camera::view() const {
  return Mat4x4f::invert(to_mat4());
}

} // namespace rx::math

#endif // RX_MATH_CAMERA_H
