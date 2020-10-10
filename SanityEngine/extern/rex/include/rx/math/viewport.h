#ifndef RX_MATH_VIEWPORT_H
#define RX_MATH_VIEWPORT_H
#include "rx/core/optional.h"

#include "rx/math/vec3.h"
#include "rx/math/vec2.h"
#include "rx/math/mat4x4.h"

namespace Rx::Math {

struct Viewport {
  constexpr Viewport(const Vec2z& _offset, const Vec2z& _dimensions, const Mat4x4f& _view_projection);

  Optional<Vec3f> screen_to_world(const Vec3f& _screen) const;
  Optional<Vec3f> world_to_screen(const Vec3f& _world) const;

  const Vec2z& offset() const &;
  const Vec2z& dimensions() const &;

  bool is_point_inside(const Vec2z& _point) const;

private:
  Vec2z m_offset;
  Vec2z m_dimensions;
  Mat4x4f m_view_projection;
  Mat4x4f m_inverse_view_projection;
};

inline constexpr Viewport::Viewport(const Vec2z& _offset, const Vec2z& _dimensions, const Mat4x4f& _view_projection)
  : m_offset{_offset}
  , m_dimensions{_dimensions}
  , m_view_projection{_view_projection}
  , m_inverse_view_projection{Mat4x4f::invert(m_view_projection)}
{
}

inline const Vec2z& Viewport::offset() const & {
  return m_offset;
}

inline const Vec2z& Viewport::dimensions() const & {
  return m_dimensions;
}

inline bool Viewport::is_point_inside(const Vec2z& _point) const {
  return _point >= m_offset && _point < m_offset + m_dimensions;
}

} // namespace Rx::Math

#endif // RX_MATH_VIEWPORT_H
