#include <stdio.h> // snprintf

#include "rx/core/math/sqrt.h"

#include "rx/math/vec2.h"

namespace Rx {

const char* FormatNormalize<Math::Vec2f>::operator()(const Math::Vec2f& value) {
  snprintf(scratch, sizeof scratch, "{%f, %f}", value.x, value.y);
  return scratch;
}

const char* FormatNormalize<Math::Vec2i>::operator()(const Math::Vec2i& value) {
  snprintf(scratch, sizeof scratch, "{%d, %d}", value.x, value.y);
  return scratch;
}

} // namespace rx
