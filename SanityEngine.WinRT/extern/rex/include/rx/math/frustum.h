#ifndef RX_MATH_FRUSTUM_H
#define RX_MATH_FRUSTUM_H
#include "rx/math/plane.h"
#include "rx/math/mat4x4.h"

namespace Rx::Math {

struct AABB;

struct Frustum {
  Frustum(const Mat4x4f& _view_projection);

  bool is_aabb_inside(const AABB& _aabb) const;

private:
  Plane m_planes[6];
};

} // namespace rx::math

#endif // RX_MATH_FRUSTUM_H
