#ifndef RX_MATH_SPHERE_H
#define RX_MATH_SPHERE_H
#include "rx/math/vec3.h"

namespace Rx::Math {

template<typename T>
struct Mat4x4;

using Mat4x4f = Mat4x4<Float32>;

struct Sphere {
  constexpr Sphere();
  constexpr Sphere(const Vec3f& _origin, Float32 _radius);

  const Math::Vec3f& origin() const &;
  Float32 radius() const;

  Sphere transform(const Mat4x4f& _mat) const;

private:
  Vec3f m_origin;
  Float32 m_radius;
};

inline constexpr Sphere::Sphere()
  : Sphere{{0.0f, 0.0f, 0.0f}, 1.0f}
{
}

inline constexpr Sphere::Sphere(const Vec3f& _origin, Float32 _radius)
  : m_origin{_origin}
  , m_radius{_radius}
{
}

inline const Math::Vec3f& Sphere::origin() const & {
  return m_origin;
}

inline Float32 Sphere::radius() const {
  return m_radius;
}

} // namespace Rx

#endif // RX_MATH_SPHERE_H
