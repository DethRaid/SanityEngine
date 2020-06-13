#ifndef RX_MATH_AABB_H
#define RX_MATH_AABB_H
#include "rx/math/vec3.h"

#include "rx/core/algorithm/min.h"
#include "rx/core/algorithm/max.h"

namespace Rx::Math {

template<typename T>
struct Mat4x4;

using Mat4x4f = Mat4x4<Float32>;

struct AABB {
  constexpr AABB();
  constexpr AABB(const Vec3f& _min, const Vec3f& _max);

  void expand(const Vec3f& _point);
  void expand(const AABB& _bounds);

  AABB transform(const Mat4x4f& _mat) const;

  const Vec3f& min() const &;
  const Vec3f& max() const &;

  Vec3f origin() const;
  Vec3f scale() const;

private:
  Vec3f m_min;
  Vec3f m_max;
};

inline constexpr AABB::AABB()
  : m_min{ FLT_MAX,  FLT_MAX,  FLT_MAX}
  , m_max{-FLT_MIN, -FLT_MAX, -FLT_MAX}
{
}

inline constexpr AABB::AABB(const Vec3f& _min, const Vec3f& _max)
  : m_min{_min}
  , m_max{_max}
{
}

inline void AABB::expand(const AABB& _bounds) {
  expand(_bounds.m_min);
  expand(_bounds.m_max);
}

inline const Vec3f& AABB::min() const & {
  return m_min;
}

inline const Vec3f& AABB::max() const & {
  return m_max;
}

inline Vec3f AABB::origin() const {
  return (m_min + m_max) * 0.5f;
}

inline Vec3f AABB::scale() const {
  return (m_max - m_min) * 0.5f;
}

}

#endif // RX_MATH_AABB_H
