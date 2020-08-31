#ifndef RX_MATH_NOISE_PERLIN_H
#define RX_MATH_NOISE_PERLIN_H
#include "rx/core/types.h"

namespace Rx::PRNG {
  struct MT19937;
}

namespace Rx::Math::noise {

struct Perlin {
  Perlin(PRNG::MT19937& _mt19937);

  Float32 noise(Float32 _x) const;
  Float32 noise(Float32 _x, Float32 _y) const;
  Float32 noise(Float32 _x, Float32 _y, Float32 _z) const;

  void reseed();

private:
  PRNG::MT19937& m_mt19937;
  Byte m_data[512];
};

inline Float32 Perlin::noise(Float32 _x) const {
  return noise(_x, 0.0f, 0.0f);
}

inline Float32 Perlin::noise(Float32 _x, Float32 _y) const {
  return noise(_x, _y, 0.0f);
}

} // namespace rx::math::noise

#endif // RX_MATH_NOISE_PERLIN_H
