#include "rx/math/mat3x4.h"
#include "rx/math/mat3x3.h"
#include "rx/math/quat.h"

namespace Rx::Math {

template<typename T>
Mat3x4<T>::Mat3x4(const Mat3x3<T>& _scale_rotate, const Vec3<T>& _translation)
  : x{_scale_rotate.x.x, _scale_rotate.x.y, _scale_rotate.x.z, _translation.x}
  , y{_scale_rotate.y.x, _scale_rotate.y.y, _scale_rotate.y.z, _translation.y}
  , z{_scale_rotate.z.x, _scale_rotate.z.y, _scale_rotate.z.z, _translation.z}
{
}

template<typename T>
Mat3x4<T>::Mat3x4(const Quat<T>& _rotation, const Vec3<T>& _translation)
  : Mat3x4{Mat3x3<T>{_rotation}, _translation}
{
}

template<typename T>
Mat3x4<T>::Mat3x4(const Vec3<T>& _scale, const Quat<T>& _rotation,
                  const Vec3<T>& _translation)
  : Mat3x4{Mat3x3<T>{_scale, _rotation}, _translation}
{
}

template Mat3x4<Float32>::Mat3x4(const Mat3x3<Float32>& _rotation,
                                const Vec3<Float32>& _translation);
template Mat3x4<Float32>::Mat3x4(const Quat<Float32>& _rotation,
                                const Vec3<Float32>& _translation);
template Mat3x4<Float32>::Mat3x4(const Vec3<Float32>& _scale,
                                const Quat<Float32>& _rotation, const Vec3<Float32>& _translation);

} // namespace rx::math
