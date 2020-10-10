#ifndef RX_MATH_RAY_H
#define RX_MATH_RAY_H
#include "rx/math/vec3.h"

namespace Rx::Math {

struct Ray {
  constexpr Ray() = default;
  constexpr Ray(const Vec3f& _point, const Vec3f& _direction);

  constexpr const Vec3f& point() const &;
  constexpr const Vec3f& direction()  const &;

  constexpr Vec3f closest_point(const Vec3f& _point) const;

private:
  Vec3f m_point;
  Vec3f m_direction;
};

inline constexpr Ray::Ray(const Vec3f& _point, const Vec3f& _direction)
  : m_point{_point}
  , m_direction{_direction}
{
}

inline constexpr const Vec3f& Ray::point() const & {
  return m_point;
}

inline constexpr const Vec3f& Ray::direction() const & {
  return m_direction;
}

inline constexpr Vec3f Ray::closest_point(const Vec3f& _point) const {
  const auto relative_point = m_point - _point;
  const auto relative_point_distance = -dot(m_direction, relative_point);
  return _point + relative_point + (m_direction * relative_point_distance);
}

} // namespace Rx::Math

#endif // RX_MATH_RAY_H
