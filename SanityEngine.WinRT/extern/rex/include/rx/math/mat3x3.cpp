#include "rx/math/mat3x3.h"
#include "rx/math/quat.h"

namespace Rx::Math {

template<typename T>
static Mat3x3<T> quat_to_mat3x3(const Quat<T>& _quat) {
  return {
    {
      T{1} - T{2}*_quat.y*_quat.y - T{2}*_quat.z*_quat.z,
             T{2}*_quat.x*_quat.y - T{2}*_quat.z*_quat.w,
             T{2}*_quat.x*_quat.z + T{2}*_quat.y*_quat.w
    },{
             T{2}*_quat.x*_quat.y + T{2}*_quat.z*_quat.w,
      T{1} - T{2}*_quat.x*_quat.x - T{2}*_quat.z*_quat.z,
             T{2}*_quat.y*_quat.z - T{2}*_quat.x*_quat.w
    },{
             T{2}*_quat.x*_quat.z - T{2}*_quat.y*_quat.w,
             T{2}*_quat.y*_quat.z + T{2}*_quat.x*_quat.w,
      T{1} - T{2}*_quat.x*_quat.x - T{2}*_quat.y*_quat.y
    }
  };
}

template<typename T>
Mat3x3<T>::Mat3x3(const Quat<T>& _rotation)
  : Mat3x3<T>{quat_to_mat3x3(_rotation)}
{
}

template<typename T>
Mat3x3<T>::Mat3x3(const Vec3<T>& _scale, const Quat<T>& _rotation)
  : Mat3x3<T>{_rotation}
{
  x *= _scale;
  y *= _scale;
  z *= _scale;
}

template Mat3x3<Float32>::Mat3x3(const Quat<Float32>& _rotation);
template Mat3x3<Float32>::Mat3x3(const Vec3<Float32>& _scale, const Quat<Float32>& _rotation);

} // namespace rx::math