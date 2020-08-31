#include "rx/math/sphere.h"
#include "rx/math/mat4x4.h"

namespace Rx::Math {

Sphere Sphere::transform(const Mat4x4f& _mat) const {
  const auto length_x = length_squared(Vec3f{_mat.x.x, _mat.x.y, _mat.x.z});
  const auto length_y = length_squared(Vec3f{_mat.y.x, _mat.y.y, _mat.y.z});
  const auto length_z = length_squared(Vec3f{_mat.z.x, _mat.z.y, _mat.z.z});

  Float32 radius = 0.0f;
  if (length_x > length_y && length_x > length_z) {
    radius = m_radius * sqrt(length_x);
  } else if (length_y > length_z) {
    radius = m_radius * sqrt(length_y);
  } else {
    radius = m_radius * sqrt(length_z);
  }

  return {Math::Mat4x4f::transform_point(m_origin, _mat), radius};
}

} // namespace Rx::Math
