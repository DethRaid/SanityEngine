#ifndef RX_MATH_RECTANGLE_H
#define RX_MATH_RECTANGLE_H
#include "rx/math/vec2.h"

namespace Rx::Math {

template<typename T>
struct Rectangle {
  Vec2<T> offset;
  Vec2<T> dimensions;

  // Checks if rectangle |_other| is entierly inside this rectangle.
  bool contains(const Rectangle& _other) const;

  // Checks if |_point| is inside this rectangle.
  bool contains(const Vec2<T>& _point) const;

  template<typename U>
  Rectangle<U> cast() const;
};

template<typename T>
inline bool Rectangle<T>::contains(const Rectangle& _other) const {
  return _other.offset + _other.dimensions < offset + dimensions && _other.offset > offset;
}

template<typename T>
inline bool Rectangle<T>::contains(const Vec2<T>& _point) const {
  return _point < offset + dimensions && _point > offset;
}

template<typename T>
template<typename U>
Rectangle<U> Rectangle<T>::cast() const {
  return {offset.template cast<U>(), dimensions.template cast<U>()};
}

} // namespace rx::math

#endif // RX_MATH_RECTANGLE_H
