#ifndef RX_MATH_RECTANGLE_H
#define RX_MATH_RECTANGLE_H
#include "rx/math/vec2.h"

namespace Rx::Math {

template<typename T>
struct Rectangle {
  Vec2<T> offset;
  Vec2<T> dimensions;

  bool contains(const Rectangle& _other) const;
};

template<typename T>
inline bool Rectangle<T>::contains(const Rectangle& _other) const {
  return _other.offset + _other.dimensions < offset + dimensions && _other.offset > offset;
}

} // namespace rx::math

#endif // RX_MATH_RECTANGLE_H
