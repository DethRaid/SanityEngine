#include "rx/math/noise/perlin.h"
#include "rx/core/math/floor.h"
#include "rx/core/prng/mt19937.h"
#include "rx/core/utility/swap.h"

namespace Rx::Math::noise {

static inline Float32 fade(Float32 _t) {
  return _t * _t * _t * (_t * (_t * 6.0f - 15.0f) + 10.0f);
}

static inline Float32 lerp(Float32 _t, Float32 _a, Float32 _b) {
  return _a + _t * (_b - _a);
}

static inline Float32 grad(Byte _hash, Float32 _x, Float32 _y, Float32 _z) {
  const Byte hash{static_cast<Byte>(_hash & 15)};
  const Float32 u{hash < 5 ? _x : _y};
  const Float32 v{hash < 4 ? _y : hash == 12 || hash == 14 ? _x : _z};
  return ((_hash & 1) == 0 ? u : -u) + ((hash & 2) == 0 ? v : -v);
}

Perlin::Perlin(PRNG::MT19937& _mt19937)
  : m_mt19937{_mt19937}
{
  reseed();
}

void Perlin::reseed() {
  for (Size i{0}; i < 256; i++) {
    m_data[i] = static_cast<Byte>(i);
  }

  Size n{256};
  for (Size i{0}; i < n - 1; i++) {
    const Size j{m_mt19937.u32() % (i + 1)};
    Utility::swap(m_data[i], m_data[j]);
  }

  for (Size i{0}; i < 256; i++) {
    m_data[256 + i] = m_data[i];
  }
}

Float32 Perlin::noise(Float32 _x, Float32 _y, Float32 _z) const {
  const Sint32 X{static_cast<Sint32>(floor(_x)) & 255};
  const Sint32 Y{static_cast<Sint32>(floor(_y)) & 255};
  const Sint32 Z{static_cast<Sint32>(floor(_z)) & 255};

  _x -= floor(_x);
  _y -= floor(_y);
  _z -= floor(_z);

  const Float32 u{fade(_x)};
  const Float32 v{fade(_y)};
  const Float32 w{fade(_z)};

  const Sint32 A{m_data[X] + Y};
  const Sint32 AA{m_data[A] + Z};
  const Sint32 AB{m_data[A + 1] + Z};

  const Sint32 B{m_data[X + 1] + Y};
  const Sint32 BA{m_data[B] + Z};
  const Sint32 BB{m_data[B + 1] + Z};

  return lerp(w, lerp(v, lerp(u, grad(m_data[AA],     _x,        _y,        _z),
                                 grad(m_data[BA],     _x - 1.0f, _y,        _z)),
                         lerp(u, grad(m_data[AB],     _x,        _y - 1.0f, _z),
                                 grad(m_data[BB],     _x - 1.0f, _y - 1.0f, _z))),
                 lerp(v, lerp(u, grad(m_data[AA + 1], _x,        _y,        _z - 1.0f),
                                 grad(m_data[BA + 1], _x - 1.0f, _y,        _z - 1.0f)),
                         lerp(u, grad(m_data[AB + 1], _x,        _y - 1.0f, _z - 1.0f),
                                 grad(m_data[BB + 1], _x - 1.0f, _y - 1.0f, _z - 1.0f))));
}

} // namespace rx::math::noise
