#ifndef RX_MATH_PLANE_H
#define RX_MATH_PLANE_H
#include "rx/math/vec3.h"

#include "rx/core/optional.h"

namespace Rx::Math {

struct Line;
struct Ray;

struct Plane {
  constexpr Plane();
  Plane(const Vec3f& _normal, Float32 _distance);
  Plane(const Vec3f& _normal, const Vec3f& _point);

  const Vec3f& normal() const &;
  Float32 distance() const;

  Optional<Vec3f> line_intersect(const Line& _line) const;
  Optional<Vec3f> ray_intersect(const Ray& _ray) const;

private:
  Vec3f m_normal;
  Float32 m_distance;
};

inline constexpr Plane::Plane()
  : m_normal{}
  , m_distance{0.0f}
{
}

inline Plane::Plane(const Vec3f& _normal, Float32 _distance)
  : m_normal{_normal}
  , m_distance{_distance}
{
  const Float32 magnitude = 1.0f / length(m_normal);
  m_normal *= magnitude;
  m_distance *= magnitude;
}

inline Plane::Plane(const Vec3f& _normal, const Vec3f& _point)
  : m_normal{normalize(_normal)}
  , m_distance{dot(m_normal, _point)}
{
}

inline const Vec3f& Plane::normal() const & {
  return m_normal;
}

inline Float32 Plane::distance() const {
  return m_distance;
}

} // namespace rx::math

#endif // RX_MATH_PLANE_H
