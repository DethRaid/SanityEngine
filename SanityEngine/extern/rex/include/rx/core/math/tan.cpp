#define _USE_MATH_DEFINES
#include <math.h> // M_PI_2

#include "rx/core/math/tan.h"
#include "rx/core/math/abs.h"
#include "rx/core/math/shape.h"
#include "rx/core/math/force_eval.h"

#include "rx/core/hints/unreachable.h"

namespace Rx::Math {

// |tan(x)/x - t(x)| < 2**-25.5 (~[-2e-08, 2e-08])
static constexpr const Float64 k_t0{0x15554d3418c99f.0p-54}; // 0.333331395030791399758
static constexpr const Float64 k_t1{0x1112fd38999f72.0p-55}; // 0.133392002712976742718
static constexpr const Float64 k_t2{0x1b54c91d865afe.0p-57}; // 0.0533812378445670393523
static constexpr const Float64 k_t3{0x191df3908c33ce.0p-58}; // 0.0245283181166547278873
static constexpr const Float64 k_t4{0x185dadfcecf44e.0p-61}; // 0.00297435743359967304927
static constexpr const Float64 k_t5{0x1362b9bf971bcd.0p-59}; // 0.00946564784943673166728

template<bool Odd>
static Float32 tandf(Float64 _x) {
  const Float64Eval z{_x * _x};
  const Float64Eval r{k_t4 + z * k_t5};
  const Float64Eval t{k_t2 + z * k_t3};
  const Float64Eval w{z * z};
  const Float64Eval s{z * _x};
  const Float64Eval u{k_t0 + z * k_t1};
  const Float64Eval l{(_x + s * u) + (s * w) * (t + w * r)};
  return static_cast<Float32>(Odd ? -1.0 / l : l);
}

// small multiples of pi/2 rounded to double precision
static constexpr const Float64 k_t1_pi_2{1 * M_PI_2};
static constexpr const Float64 k_t2_pi_2{2 * M_PI_2};
static constexpr const Float64 k_t3_pi_2{3 * M_PI_2};
static constexpr const Float64 k_t4_pi_2{4 * M_PI_2};

Sint32 rempio2(Float32 _x, Float64& y_); // sin.cpp

Float32 tan(Float32 _x) {
  Uint32 ix{Shape{_x}.as_u32};
  const Uint32 sign{ix >> 31};

  ix &= 0x7fffffff;

  // |_x| ~<= pi/4
  if (ix <= 0x3f490fda) {
    // |_x| < 2**-12
    if (ix < 0x39800000) {
      // raise inexact if x != 0 and underflow if subnormal
      force_eval_f32(ix < 0x00800000 ? _x/0x1p120f : _x+0x1p120f);
      return _x;
    }
    return tandf<false>(_x);
  }

  // |_x| ~<= 5*pi/4
  if (ix <= 0x407b53d1) {
    // |_x| ~<= 3*pi/4
    if (ix <= 0x4016cbe3) {
      return tandf<true>(sign ? _x+k_t1_pi_2 : _x-k_t1_pi_2);
    } else {
      return tandf<false>(sign ? _x+k_t2_pi_2 : _x-k_t2_pi_2);
    }
  }

  // |_x| ~<= 9*pi/4
  if (ix <= 0x40e231d5) {
    // |_x| ~<= 7*pi/4
    if (ix <= 0x40afeddf) {
      return tandf<true>(sign ? _x+k_t3_pi_2 : _x-k_t3_pi_2);
    } else {
      return tandf<false>(sign ? _x+k_t4_pi_2 : _x-k_t4_pi_2);
    }
  }

  // tan(+inf) = NaN
  // tan(-inf) = NaN
  // tan(NaN) = NaN
  if (ix >= 0x7f800000) {
    return _x - _x;
  }

  Float64 y;
  const Sint32 n{rempio2(_x, y)};
  if (n&1) {
    return tandf<true>(y);
  } else {
    return tandf<false>(y);
  }

  RX_HINT_UNREACHABLE();
}

static constexpr const Float32 k_atanhi[] = {
  4.6364760399e-01, // atan(0.5) hi 0x3eed6338
  7.8539812565e-01, // atan(1.0) hi 0x3f490fda
  9.8279368877e-01, // atan(1.5) hi 0x3f7b985e
  1.5707962513e+00, // atan(inf) hi 0x3fc90fda
};

static constexpr const Float32 k_atanlo[] = {
  5.0121582440e-09, // atan(0.5) lo 0x31ac3769
  3.7748947079e-08, // atan(1.0) lo 0x33222168
  3.4473217170e-08, // atan(1.5) lo 0x33140fb4
  7.5497894159e-08, // atan(inf) lo 0x33a22168
};

static constexpr const Float32 k_aT[] = {
  3.3333328366e-01,
 -1.9999158382e-01,
  1.4253635705e-01,
 -1.0648017377e-01,
  6.1687607318e-02,
};

Float32 atan(Float32 _x) {
  const Uint32 ix = Shape{_x}.as_u32;
  const Uint32 sign = (ix >> 31) & 0x7fffffff;

  // if |_x| >= 2**26
  if (ix >= 0x4c800000) {
    if (isnan(_x)) {
      return _x;
    }

    const Float32Eval z = k_atanhi[3] + 0x1p-120f;
    return sign ? -z : z;
  }

  int id = 0;

  // |_x| < 0.4375
  if (ix < 0x3ee00000) {
    // |_x| < 2**-12
    if (ix < 0x39800000) {
      if (ix < 0x00800000) {
        // raise underflow for subnormal _x
        force_eval_f32(_x * _x);
      }
      return _x;
    }
    id = -1;
  } else {
    _x = abs(_x);
    // |_x| < 1.1875
    if (ix < 0x3f980000) {
      // 7/16 <= |_x| < 11/16
      if (ix < 0x3f300000) {
        id = 0;
        _x = (2.0f * _x - 1.0f) / (2.0f + _x);
      } else {
        // 11/16 <= |_x| < 19/16
        id = 1;
        _x = (_x - 1.0f) / (_x + 1.0f);
      }
    } else {
      // |_x| < 2.4375
      if (ix < 0x401c0000) {
        id = 2;
        _x = (_x - 1.5f) / (1.0f + 1.5f*_x);
      } else {
        // 2.4375 <= |_x| < 2**26
        id = 3;
        _x = -1.0f/_x;
      }
    }
  }

  // end of argument reduction
  Float32Eval z = _x * _x;
  Float32Eval w = z * z;

  // break sum from i=0 to 10 aT[i]z**(i+1) into odd and even poly
  Float32Eval s1 = z * (k_aT[0] + w * (k_aT[2] + w * k_aT[4]));
  Float32Eval s2 = w * (k_aT[1] + w * k_aT[3]);
  if (id < 0) {
    return _x - _x * (s1 + s2);
  }

  z = k_atanhi[id] - ((_x * (s1 + s2) - k_atanlo[id]) - _x);
  return sign ? -z : z;
}

static constexpr const Float32 k_pi = 3.1415927410e+00; // 0x40490fdb
static constexpr const Float32 k_pi_lo = -8.7422776573e-08; // 0xb3bbbd2e

Float32 atan2(Float32 _x, Float32 _y) {
  if (isnan(_x) || isnan(_y)) {
    return _x + _y;
  }

  Uint32 ix = Shape{_x}.as_u32;
  Uint32 iy = Shape{_y}.as_u32;

  // _x = 1.0
  if (ix == 0x3f800000) {
    return atan(_y);
  }

  // 2 * sign(_x) + sign(_y)
  const Uint32 m = ((iy >> 31) & 1) | ((ix >> 30) & 2);

  ix &= 0x7fffffff;
  iy &= 0x7fffffff;

  // when y = 0
  if (iy == 0) {
    switch (m) {
    case 0:
      [[fallthrough]];
    case 1:
      // atan(+-0, +anything) = +-0
      return _y;
    case 2:
      // atan(+0, -anything) = pi
      return k_pi;
    case 3:
      //  atan(-0, -anything) = -pi
      return -k_pi;
    }
  }

  // when x = 0
  if (ix == 0) {
    return m&1 ? -k_pi / 2 : k_pi / 2;
  }

  // when x is INF
  if (ix == 0x7f800000) {
    if (iy == 0x7f800000) {
      switch (m) {
      case 0:
        // atan(+INF, +INF)
        return k_pi / 4;
      case 1:
        // atan(-INF, +INF)
        return -k_pi/4; /* */
      case 2:
        // atan(+INF, -INF)
        return 3 * k_pi / 4;
      case 3:
        // atan(-INF, -INF)
        return -3 * k_pi / 4;
      }
    } else {
      switch (m) {
      case 0:
        // atan(+..., +INF)
        return 0.0f;
      case 1:
        // atan(-..., +INF)
        return -0.0f;
      case 2:
        // atan(+..., -INF)
        return k_pi;
      case 3:
        // atan(-..., -INF)
        return -k_pi;
      }
    }
  }

  // |y/x| > 0x1p26
  if (ix + (26 << 23) < iy || iy == 0x7f800000) {
    return m&1 ? -k_pi / 2 : k_pi / 2;
  }

  // z = atan(|y/x|) with correct underflow

  // |y/x| < 0x1p-26, x < 0
  Float32 z;
  if ((m & 2) && iy + (26 << 23) < ix) {
    z = 0.0;
  } else {
    z = atan(fabs(_y / _x));
  }

  switch (m) {
  case 0:
    // atan(+, +)
    return z;
  case 1:
    // atan(-, +)
    return -z;
  case 2:
    // atan(+, -)
    return k_pi - (z - k_pi_lo);
  default:
    // atan(-, -)
    return (z - k_pi_lo) - k_pi;
  }

  RX_HINT_UNREACHABLE();
}

} // namespace rx::math
