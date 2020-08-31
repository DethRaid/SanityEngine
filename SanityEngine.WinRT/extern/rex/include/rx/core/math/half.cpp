#include "rx/core/math/half.h" // half
#include "rx/core/math/shape.h" // shape

#include "rx/core/global.h"

namespace Rx::Math {

static constexpr const Uint32 k_magic = 113 << 23;
static constexpr const Uint32 k_shift_exp = 0x7C00 << 13; // exp mask after shift
static constexpr Shape<Float32> k_magic_bits = k_magic;

struct HalfTable {
  HalfTable() {
    for (int i{0}, e{0}; i < 256; i++) {
      e = i - 127;
      if (e < -24) {
        base[i|0x000] = 0x0000;
        base[i|0x100] = 0x8000;
        shift[i|0x000] = 24;
        shift[i|0x100] = 24;
      } else if (e < -14) {
        base[i|0x000] = 0x0400 >> (-e-14);
        base[i|0x100] = (0x0400 >> (-e-14)) | 0x8000;
        shift[i|0x000] = -e-1;
        shift[i|0x100] = -e-1;
      } else if (e <= 15) {
        base[i|0x000] = (e+15) << 10;
        base[i|0x100] = ((e+15) << 10) | 0x8000;
        shift[i|0x000] = 13;
        shift[i|0x100] = 13;
      } else if (e < 128) {
        base[i|0x000] = 0x7C00;
        base[i|0x100] = 0xFC00;
        shift[i|0x000] = 24;
        shift[i|0x100] = 24;
      } else {
        base[i|0x000] = 0x7C00;
        base[i|0x100] = 0xFC00;
        shift[i|0x000] = 13;
        shift[i|0x100] = 13;
      }
    }
  }

  Uint32 base[512];
  Uint8 shift[512];
};

static const Global<HalfTable> g_table{"system", "half"};

Half Half::to_half(Float32 _f) {
  const Shape u{_f};
  return Half(static_cast<Uint16>(g_table->base[(u.as_u32 >> 23) & 0x1FF] +
                                  ((u.as_u32 & 0x007FFFFF) >> g_table->shift[(u.as_u32 >> 23) & 0x1FF])));
}

Float32 Half::to_f32() const {
  Shape out{static_cast<Uint32>((m_bits & 0x7FFF) << 13)}; // exp/mantissa
  const auto exp{k_shift_exp & out.as_u32}; // exp
  out.as_u32 += (127 - 15) << 23; // adjust exp
  if (exp == k_shift_exp) {
    out.as_u32 += (128 - 16) << 23; // adjust for inf/nan
  } else if (exp == 0) {
    out.as_u32 += 1 << 23; // adjust for zero/denorm
    out.as_f32 -= k_magic_bits.as_f32; // renormalize
  }
  out.as_u32 |= (m_bits & 0x8000) << 16; // sign bit
  return out.as_f32;
}

} // namespace rx::math
