#include <stdio.h> // snprintf

#include "rx/core/math/sqrt.h"

#include "rx/math/vec3.h" // vec3

namespace Rx {

const char* FormatNormalize<Math::Vec3f>::operator()(const Math::Vec3f& value) {
  snprintf(scratch, sizeof scratch, "{%f, %f, %f}", value.x, value.y, value.z);
  return scratch;
}

const char* FormatNormalize<Math::Vec3i>::operator()(const Math::Vec3i& value) {
  snprintf(scratch, sizeof scratch, "{%d, %d, %d}", value.x, value.y, value.z);
  return scratch;
}

} // namespace rx
