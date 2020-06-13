#include "rx/math/aabb.h"
#include "rx/math/mat4x4.h"

namespace Rx::Math {

void AABB::expand(const Vec3f& _point) {
  m_min.x = Algorithm::min(_point.x, m_min.x);
  m_min.y = Algorithm::min(_point.y, m_min.y);
  m_min.z = Algorithm::min(_point.z, m_min.z);

  m_max.x = Algorithm::max(_point.x, m_max.x);
  m_max.y = Algorithm::max(_point.y, m_max.y);
  m_max.z = Algorithm::max(_point.z, m_max.z);
}

AABB AABB::transform(const Mat4x4f& _mat) const {
  const Vec3f& x{_mat.x.x, _mat.x.y, _mat.x.z};
  const Vec3f& y{_mat.y.x, _mat.y.y, _mat.y.z};
  const Vec3f& z{_mat.z.x, _mat.z.y, _mat.z.z};
  const Vec3f& w{_mat.w.x, _mat.w.y, _mat.w.z};

  const Vec3f& xa{x * m_min.x};
  const Vec3f& xb{x * m_max.x};
  const Vec3f& ya{y * m_min.y};
  const Vec3f& yb{y * m_max.y};
  const Vec3f& za{z * m_min.z};
  const Vec3f& zb{z * m_max.z};

  const auto min{[](const Vec3f& _lhs, const Vec3f& _rhs) -> Vec3f {
    return {
      Algorithm::min(_lhs.x, _rhs.x),
      Algorithm::min(_lhs.y, _rhs.y),
      Algorithm::min(_lhs.z, _rhs.z)
    };
  }};

  const auto max{[](const Vec3f& _lhs, const Vec3f& _rhs) -> Vec3f {
    return {
      Algorithm::max(_lhs.x, _rhs.x),
      Algorithm::max(_lhs.y, _rhs.y),
      Algorithm::max(_lhs.z, _rhs.z)
    };
  }};

  return {
    min(xa, xb) + min(ya, yb) + min(za, zb) + w,
    max(xa, xb) + max(ya, yb) + max(za, zb) + w
  };
}

} // namespace rx::math
