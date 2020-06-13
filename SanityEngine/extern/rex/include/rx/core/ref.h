#ifndef RX_CORE_REF_H
#define RX_CORE_REF_H

namespace Rx {

template<typename T>
struct Ref {
  constexpr Ref(T& _ref);
  constexpr Ref(T&&) = delete;

  constexpr operator T&() const;
  constexpr T& get() const;

private:
  T* m_data;
};

template<typename T>
inline constexpr Ref<T>::Ref(T& _ref)
  : m_data{&_ref}
{
}

template<typename T>
inline constexpr Ref<T>::operator T&() const {
  return *m_data;
}

template<typename T>
inline constexpr T& Ref<T>::get() const {
  return *m_data;
}

} // namespace rx

#endif // RX_CORE_REF_H
