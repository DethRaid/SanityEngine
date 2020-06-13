#ifndef RX_CORE_MATH_HALF_H
#define RX_CORE_MATH_HALF_H
#include "rx/core/types.h"
#include "rx/core/assert.h" // RX_ASSERT

namespace Rx::Math {

struct Half {
  constexpr Half(const Half& _h);
  constexpr Half& operator=(const Half& _h);

  Half(Float32 _f);
  Half(Float64 _f);

  Float32 to_f32() const;
  Float64 to_f64() const;

  friend Half operator+(Half _lhs, Half _rhs);
  friend Half operator-(Half _lhs, Half _rhs);
  friend Half operator*(Half _lhs, Half _rhs);
  friend Half operator/(Half _lhs, Half _rhs);

  friend Half& operator+=(Half& _lhs, Half _rhs);
  friend Half& operator-=(Half& _lhs, Half _rhs);
  friend Half& operator*=(Half& _lhs, Half _rhs);
  friend Half& operator/=(Half& _lhs, Half _rhs);

  friend Half operator-(Half _h);
  friend Half operator+(Half _h);

private:
  constexpr explicit Half(Uint16 _bits);

  Half to_half(Float32 _f);
  Uint16 m_bits;
};

inline constexpr Half::Half(const Half& _h)
  : m_bits{_h.m_bits}
{
}

inline constexpr Half& Half::operator=(const Half& _h) {
  RX_ASSERT(&_h != this, "self assignment");

  m_bits = _h.m_bits;
  return *this;
}

inline Half::Half(Float32 _f)
  : Half{to_half(_f)}
{
}

inline Half::Half(Float64 _f)
  : Half{static_cast<Float32>(_f)}
{
}

inline Float64 Half::to_f64() const {
  return static_cast<Float64>(to_f32());
}

inline Half operator+(Half _lhs, Half _rhs) {
  return _lhs.to_f32() + _rhs.to_f32();
}

inline Half operator-(Half _lhs, Half _rhs) {
  return _lhs.to_f32() - _rhs.to_f32();
}

inline Half operator*(Half _lhs, Half _rhs) {
  return _lhs.to_f32() * _rhs.to_f32();
}

inline Half operator/(Half _lhs, Half _rhs) {
  return _lhs.to_f32() / _rhs.to_f32();
}

inline Half& operator+=(Half& _lhs, Half _rhs) {
  return _lhs = _lhs + _rhs;
}

inline Half& operator-=(Half& _lhs, Half _rhs) {
  return _lhs = _lhs - _rhs;
}

inline Half& operator*=(Half& _lhs, Half _rhs) {
  return _lhs = _lhs * _rhs;
}

inline Half& operator/=(Half& _lhs, Half _rhs) {
  return _lhs = _lhs / _rhs;
}

inline Half operator-(Half _h) {
  return -_h.to_f32();
}

inline Half operator+(Half _h) {
  return +_h.to_f32();
}

inline constexpr Half::Half(Uint16 _bits)
  : m_bits{_bits}
{
}

} // namespace rx::math

#endif // RX_CORE_MATH_HALF_H
