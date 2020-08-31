#include "rx/core/math/sqrt.h"

#include "rx/math/quat.h"
#include "rx/math/mat3x3.h"
#include "rx/math/mat3x4.h"

namespace Rx::Math {

template<typename T, template<class> typename M>
static Quat<T> matrix_to_quat(const M<T>& _mat) {
  const Float32 trace{_mat.x.x + _mat.y.y + _mat.z.z};

  if (trace > 0.0f) {
    const Float32 r{sqrt(1.0f + trace)};
    const Float32 i{0.5f / r};

    return {
      (_mat.z.y - _mat.y.z) * i,
      (_mat.x.z - _mat.z.x) * i,
      (_mat.y.x - _mat.x.y) * i,
      0.5f * r
    };
  } else if (_mat.x.x > _mat.y.y && _mat.x.x > _mat.z.x) {
    const Float32 r{sqrt(1.0f + _mat.x.x - _mat.y.y - _mat.z.z)};
    const Float32 i{0.5f / r};

    return {
      0.5f * r,
      (_mat.y.x + _mat.x.y) * i,
      (_mat.x.z + _mat.z.x) * i,
      (_mat.z.y - _mat.y.z) * i
    };
  } else if (_mat.y.y > _mat.z.z) {
    const Float32 r{sqrt(1.0f + _mat.y.y - _mat.x.x - _mat.z.z)};
    const Float32 i{0.5f / r};

    return {
      (_mat.y.x + _mat.x.y) * i,
      0.5f * r,
      (_mat.z.y + _mat.y.z) * i,
      (_mat.x.z - _mat.z.x) * i
    };
  }

  const Float32 r{sqrt(1.0f + _mat.z.z - _mat.x.x - _mat.y.y)};
  const Float32 i{0.5f / r};

  return {
    (_mat.x.z + _mat.z.x) * i,
    (_mat.z.y + _mat.y.z) * i,
    0.5f * r,
    (_mat.y.x - _mat.x.y) * i
  };
}

template<typename T>
Quat<T>::Quat(const Mat3x3<T>& _mat)
  : Quat{matrix_to_quat(_mat)}
{
}

template<typename T>
Quat<T>::Quat(const Mat3x4<T>& _mat)
  : Quat{matrix_to_quat(_mat)}
{
}

template Quat<Float32>::Quat(const Mat3x3<Float32>& _mat);
template Quat<Float32>::Quat(const Mat3x4<Float32>& _mat);

Float32 length(const Quatf& _value) {
  return sqrt(dot(_value, _value));
}

Quatf normalize(const Quatf& _value) {
  return _value * (1.0f / Math::length(_value));
}

} // namespace rx::math