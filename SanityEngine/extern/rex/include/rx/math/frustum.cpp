#include "rx/math/frustum.h"
#include "rx/math/aabb.h"

namespace Rx::Math {

Frustum::Frustum(const Mat4x4f& _view_projection) {
  const Vec4f& x{_view_projection.x};
  const Vec4f& y{_view_projection.y};
  const Vec4f& z{_view_projection.z};
  const Vec4f& w{_view_projection.w};

  m_planes[0] = {{x.w + x.x, y.w + y.x, z.w + z.x}, -(w.w + w.x)};
  m_planes[1] = {{x.w - x.x, y.w - y.x, z.w - z.x}, -(w.w - w.x)};
  m_planes[2] = {{x.w - x.y, y.w - y.y, z.w - z.y}, -(w.w - w.y)};
  m_planes[3] = {{x.w + x.y, y.w + y.y, z.w + z.y}, -(w.w + w.y)};
  m_planes[4] = {{x.w - x.z, y.w - y.z, z.w - z.z}, -(w.w - w.z)};
  m_planes[5] = {{x.w + x.z, y.w + y.z, z.w + z.z}, -(w.w + w.z)};
}

bool Frustum::is_aabb_inside(const AABB& _aabb) const {
  const auto& min{_aabb.min()};
  const auto& max{_aabb.max()};
  for (Size i{0}; i < 6; i++) {
    const auto& plane{m_planes[i]};
    const auto& normal{plane.normal()};
    const Float32 distance{normal.x * (normal.x < 0.0f ? min.x : max.x) +
                          normal.y * (normal.y < 0.0f ? min.y : max.y) +
                          normal.z * (normal.z < 0.0f ? min.z : max.z)};
    if (distance <= plane.distance()) {
      return false;
    }
  }
  return true;
}

} // namespace rx::math
