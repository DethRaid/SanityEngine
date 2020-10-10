#include "rx/math/viewport.h"

namespace Rx::Math {

Optional<Vec3f> Viewport::screen_to_world(const Vec3f& _screen) const {
  const Vec4f screen_space = {
    (                 (_screen.x - m_offset.x)) / m_dimensions.x * 2.0f - 1.0f,
    (m_dimensions.y - (_screen.y - m_offset.y)) / m_dimensions.y * 2.0f - 1.0f,
    (                 (_screen.z))                               * 2.0f - 1.0f,
    1.0f
  };

  const Vec4f world_space =
    Mat4x4f::transform_vector(screen_space, m_inverse_view_projection);

  // Point is behind.
  if (world_space.w < 0.0f) {
    return nullopt;
  }

  // Prevent division by zero.
  const auto w = world_space.w == 0.0f ? 1.0f : 1.0f / world_space.w;

  // Perspective division by w.
  const auto x = world_space.x * w;
  const auto y = world_space.y * w;
  const auto z = world_space.z * w;

  return Vec3f{x, y, z};
}

Optional<Vec3f> Viewport::world_to_screen(const Vec3f& _world) const {
  const Vec4f screen_space =
    Mat4x4f::transform_vector({_world.x, _world.y, _world.z, 1.0f}, m_view_projection);

  // Check if behind.
  if (screen_space.w < 0.0f) {
    return nullopt;
  }

  // Prevent division by zero.
  const auto w = screen_space.w == 0.0f ? 1.0f : 1.0f / screen_space.w;

  // Perspective division by w.
  const auto x = screen_space.x * w;
  const auto y = screen_space.y * w;
  const auto z = screen_space.z * w;

  return Vec3f{
                     (1.0f + x) * 0.5f * m_dimensions.x + m_offset.x,
    m_dimensions.y - (1.0f + y) * 0.5f * m_dimensions.x + m_offset.y,
                     (1.0f + z) * 0.5f
  };
}

} // namespace Rx::Math
