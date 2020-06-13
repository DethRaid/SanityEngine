#include <stdio.h> // snprintf

#include "rx/core/math/sqrt.h"

#include "rx/math/vec4.h" // vec4

namespace Rx {

const char* FormatNormalize<Math::Vec4f>::operator()(const Math::Vec4f& value) {
  snprintf(scratch, sizeof scratch, "{%f, %f, %f, %f}", value.x, value.y,
    value.z, value.w);
  return scratch;
}

const char* FormatNormalize<Math::Vec4i>::operator()(const Math::Vec4i& value) {
  snprintf(scratch, sizeof scratch, "{%d, %d, %d, %d}", value.x, value.y,
    value.z, value.w);
  return scratch;
}

} // namespace rx
