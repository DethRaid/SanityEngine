#include "rx/math/plane.h"
#include "rx/math/line.h"
#include "rx/math/ray.h"
#include "rx/math/compare.h"

namespace Rx::Math {

Optional<Vec3f> Plane::line_intersect(const Line& _line) const {
  const auto start_distance = dot(m_normal, _line.start);
  const auto end_distance = dot(m_normal, _line.end);

  if (start_distance < m_distance && end_distance < m_distance) {
    return nullopt;
  }

  if (start_distance > m_distance && end_distance > m_distance) {
    return nullopt;
  }

  const auto delta = start_distance - end_distance;
  const auto fraction = abs(delta) >= k_epsilon<Float32>
    ? (start_distance - m_distance) / delta : 0.0f;

  return _line.start + (_line.end - _line.start) * fraction;
}

Optional<Vec3f> Plane::ray_intersect(const Ray& _ray) const {
  const auto denom = dot(m_normal, _ray.direction());

  if (denom > -k_epsilon<Float32> && denom < k_epsilon<Float32>) {
    return nullopt;
  }

  const auto fraction = dot((m_normal * m_distance - _ray.point()), m_normal) / denom;

  return _ray.point() + _ray.direction() * fraction;
}

} // namespace Rx::Math
