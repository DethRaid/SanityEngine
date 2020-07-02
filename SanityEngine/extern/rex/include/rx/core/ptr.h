#ifndef RX_CORE_PTR_H
#define RX_CORE_PTR_H
#include "rx/core/markers.h"
#include "rx/core/assert.h"
#include "rx/core/hash.h"

#include "rx/core/memory/system_allocator.h"

#include "rx/core/utility/exchange.h"

namespace Rx {

// # Unique pointer
//
// Owning smart-pointer Type that releases the data when the object goes out of
// scope. Move-only Type.
//
// Since all allocations in Rex are associated with a given allocator, this must
// be given the allocator that allocated the pointer to take ownership of it.
//
// You may use the make_ptr helper to construct a ptr.
//
// There is no support for a custom deleter.
// There is no support for array types, use Ptr<array<T[E]>> instead.
//
// 32-bit: 8 bytes
// 64-bit: 16 bytes
template<typename T>
struct Ptr {
  RX_MARK_NO_COPY(Ptr);

  constexpr Ptr();
  constexpr Ptr(Memory::Allocator& _allocator);
  constexpr Ptr(Memory::Allocator& _allocator, NullPointer);

  template<typename U>
  constexpr Ptr(Memory::Allocator& _allocator, U* _data);

  template<typename U>
  Ptr(Ptr<U>&& other_);
  ~Ptr();

  template<typename U>
  Ptr& operator=(Ptr<U>&& other_);
  Ptr& operator=(NullPointer);

  template<typename U>
  void reset(Memory::Allocator& _allocator, U* _data);

  T* release();

  T& operator*() const;
  T* operator->() const;

  operator bool() const;
  T* get() const;

  constexpr Memory::Allocator& allocator() const;

  constexpr Size hash() const;

private:
  void destroy();

  template<typename U>
  friend struct Ptr;

  Memory::Allocator* m_allocator;
  T* m_data;
};

template<typename T>
inline constexpr Ptr<T>::Ptr()
  : Ptr{Memory::SystemAllocator::instance()}
{
}

template<typename T>
inline constexpr Ptr<T>::Ptr(Memory::Allocator& _allocator)
  : m_allocator{&_allocator}
  , m_data{nullptr}
{
}

template<typename T>
inline constexpr Ptr<T>::Ptr(Memory::Allocator& _allocator, NullPointer)
  : Ptr{_allocator}
{
}

template<typename T>
template<typename U>
inline constexpr Ptr<T>::Ptr(Memory::Allocator& _allocator, U* _data)
  : m_allocator{&_allocator}
  , m_data{_data}
{
}

template<typename T>
template<typename U>
inline Ptr<T>::Ptr(Ptr<U>&& other_)
  : m_allocator{other_.m_allocator}
  , m_data{Utility::exchange(other_.m_data, nullptr)}
{
}

template<typename T>
inline Ptr<T>::~Ptr() {
  destroy();
}

template<typename T>
template<typename U>
inline Ptr<T>& Ptr<T>::operator=(Ptr<U>&& ptr_) {
  // The casts are necessary here since T and U may not be the same.
  RX_ASSERT(reinterpret_cast<UintPtr>(&ptr_)
    != reinterpret_cast<UintPtr>(this), "self assignment");
  destroy();
  m_allocator = ptr_.m_allocator;
  m_data = Utility::exchange(ptr_.m_data, nullptr);
  return *this;
}

template<typename T>
inline Ptr<T>& Ptr<T>::operator=(NullPointer) {
  destroy();
  m_data = nullptr;
  return *this;
}

template<typename T>
template<typename U>
inline void Ptr<T>::reset(Memory::Allocator& _allocator, U* _data) {
  destroy();
  m_allocator = _allocator;
  m_data = _data;
}

template<typename T>
inline T* Ptr<T>::release() {
  return Utility::exchange(m_data, nullptr);
}

template<typename T>
inline T& Ptr<T>::operator*() const {
  RX_ASSERT(m_data, "nullptr");
  return *m_data;
}

template<typename T>
RX_HINT_FORCE_INLINE T* Ptr<T>::operator->() const {
  return m_data;
}

template<typename T>
RX_HINT_FORCE_INLINE Ptr<T>::operator bool() const {
  return m_data != nullptr;
}

template<typename T>
RX_HINT_FORCE_INLINE T* Ptr<T>::get() const {
  return m_data;
}

template<typename T>
RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Ptr<T>::allocator() const {
  return *m_allocator;
}

template<typename T>
inline constexpr Size Ptr<T>::hash() const {
  return Rx::Hash<T*>{}(m_data);
}

template<typename T>
inline void Ptr<T>::destroy() {
  allocator().template destroy<T>(m_data);
}

// Helper function to make a unique ptr.
template<typename T, typename... Ts>
inline Ptr<T> make_ptr(Memory::Allocator& _allocator, Ts&&... _arguments) {
  return {_allocator, _allocator.create<T>(Utility::forward<Ts>(_arguments)...)};
}

} // namespace rx

#endif // RX_CORE_PTR_H
