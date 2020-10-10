#ifndef RX_CORE_VECTOR_H
#define RX_CORE_VECTOR_H
#include "rx/core/array.h"

#include "rx/core/traits/is_same.h"
#include "rx/core/traits/is_trivially_copyable.h"
#include "rx/core/traits/is_trivially_destructible.h"
#include "rx/core/traits/return_type.h"

#include "rx/core/utility/exchange.h"
#include "rx/core/utility/uninitialized.h"

#include "rx/core/hints/restrict.h"
#include "rx/core/hints/unlikely.h"
#include "rx/core/hints/unreachable.h"

#include "rx/core/memory/system_allocator.h" // memory::{system_allocator, allocator}

namespace Rx {

namespace detail {
  void copy(void *RX_HINT_RESTRICT dst_, const void* RX_HINT_RESTRICT _src, Size _size);
}

// 32-bit: 16 bytes
// 64-bit: 32 bytes
template<typename T>
struct Vector {
  template<typename U, Size E>
  using Initializers = Array<U[E]>;

  static constexpr const Size k_npos{-1_z};

  constexpr Vector();
  constexpr Vector(Memory::Allocator& _allocator);
  constexpr Vector(Memory::View _view);

  // Construct a vector from an array of initializers. This is similar to
  // how initializer_list works in C++11 except it requires no compiler proxy
  // and is actually faster since the initializer Type can be moved.
  template<typename U, Size E>
  Vector(Memory::Allocator& _allocator, Initializers<U, E>&& _initializers);
  template<typename U, Size E>
  Vector(Initializers<U, E>&& _initializers);

  Vector(Memory::Allocator& _allocator, Size _size, Utility::UninitializedTag);
  Vector(Memory::Allocator& _allocator, Size _size);
  Vector(Memory::Allocator& _allocator, const Vector& _other);
  Vector(Size _size);
  Vector(const Vector& _other);
  Vector(Vector&& other_);

  ~Vector();

  Vector& operator=(const Vector& _other);
  Vector& operator=(Vector&& other_);

  T& operator[](Size _index);
  const T& operator[](Size _index) const;

  // resize to |size| with |value| for new objects
  bool resize(Size _size, const T& _value = {});

  // Resize of |_size| where the contents stays uninitialized.
  // This should only be used with trivially copyable T.
  bool resize(Size _size, Utility::UninitializedTag);

  // reserve |size| elements
  bool reserve(Size _size);

  bool append(const Vector& _other);

  void clear();

  Size find(const T& _value) const;

  template<typename F>
  Size find_if(F&& _compare) const;

  // append |data| by copy
  bool push_back(const T& _data);
  // append |data| by move
  bool push_back(T&& data_);

  void pop_back();

  // append new |T| construct with |args|
  template<typename... Ts>
  bool emplace_back(Ts&&... _args);

  Size size() const;
  Size capacity() const;

  bool in_range(Size _index) const;
  bool is_empty() const;

  // enumerate collection either forward or reverse
  template<typename F>
  bool each_fwd(F&& _func);
  template<typename F>
  bool each_rev(F&& _func);
  template<typename F>
  bool each_fwd(F&& _func) const;
  template<typename F>
  bool each_rev(F&& _func) const;

  void erase(Size _from, Size _to);

  // first or last element
  const T& first() const;
  T& first();
  const T& last() const;
  T& last();

  const T* data() const;
  T* data();

  constexpr Memory::Allocator& allocator() const;

  Memory::View disown();

private:
  // NOTE(dweiler): This does not adjust m_size, it only adjusts capacity.
  bool grow_or_shrink_to(Size _size);

  Memory::Allocator* m_allocator;
  T* m_data;
  Size m_size;
  Size m_capacity;
};

template<typename T>
inline constexpr Vector<T>::Vector()
  : Vector{Memory::SystemAllocator::instance()}
{
}

template<typename T>
inline constexpr Vector<T>::Vector(Memory::Allocator& _allocator)
  : m_allocator{&_allocator}
  , m_data{nullptr}
  , m_size{0}
  , m_capacity{0}
{
}

template<typename T>
inline constexpr Vector<T>::Vector(Memory::View _view)
  : m_allocator{_view.owner}
  , m_data{reinterpret_cast<T*>(_view.data)}
  , m_size{_view.size / sizeof(T)}
  , m_capacity{m_size}
{
}

template<typename T>
template<typename U, Size E>
inline Vector<T>::Vector(Memory::Allocator& _allocator, Initializers<U, E>&& _initializers)
  : Vector{_allocator}
{
  grow_or_shrink_to(E);
  if constexpr(traits::is_trivially_copyable<T>) {
    detail::copy(m_data, _initializers.data(), sizeof(T) * E);
  } else for (Size i = 0; i < E; i++) {
    Utility::construct<T>(m_data + i, Utility::move(_initializers[i]));
  }
  m_size = E;
}

template<typename T>
template<typename U, Size E>
inline Vector<T>::Vector(Initializers<U, E>&& _initializers)
  : Vector{Memory::SystemAllocator::instance(), Utility::move(_initializers)}
{
}

template<typename T>
inline Vector<T>::Vector(Memory::Allocator& _allocator, Size _size, Utility::UninitializedTag)
  : m_allocator{&_allocator}
  , m_data{nullptr}
  , m_size{_size}
  , m_capacity{_size}
{
  static_assert(traits::is_trivially_copyable<T>,
    "T isn't trivial, cannot leave uninitialized");

  m_data = reinterpret_cast<T*>(allocator().allocate(sizeof(T), m_size));
  RX_ASSERT(m_data, "out of memory");
}

template<typename T>
inline Vector<T>::Vector(Memory::Allocator& _allocator, Size _size)
  : m_allocator{&_allocator}
  , m_data{nullptr}
  , m_size{_size}
  , m_capacity{_size}
{
  m_data = reinterpret_cast<T*>(allocator().allocate(sizeof(T), m_size));
  RX_ASSERT(m_data, "out of memory");

  // TODO(dweiler): is_trivial trait so we can memset this.
  for (Size i = 0; i < m_size; i++) {
    Utility::construct<T>(m_data + i);
  }
}

template<typename T>
inline Vector<T>::Vector(Memory::Allocator& _allocator, const Vector& _other)
  : m_allocator{&_allocator}
  , m_data{nullptr}
  , m_size{_other.m_size}
  , m_capacity{_other.m_capacity}
{
  m_data = reinterpret_cast<T*>(allocator().allocate(sizeof(T), _other.m_capacity));
  RX_ASSERT(m_data, "out of memory");

  if (m_size) {
    if constexpr(traits::is_trivially_copyable<T>) {
      detail::copy(m_data, _other.m_data, _other.m_size * sizeof *m_data);
    } else for (Size i = 0; i < m_size; i++) {
      Utility::construct<T>(m_data + i, _other.m_data[i]);
    }
  }
}

template<typename T>
inline Vector<T>::Vector(Size _size)
  : Vector{Memory::SystemAllocator::instance(), _size}
{
}

template<typename T>
inline Vector<T>::Vector(const Vector& _other)
  : Vector{*_other.m_allocator, _other}
{
}

template<typename T>
inline Vector<T>::Vector(Vector&& other_)
  : m_allocator{other_.m_allocator}
  , m_data{Utility::exchange(other_.m_data, nullptr)}
  , m_size{Utility::exchange(other_.m_size, 0)}
  , m_capacity{Utility::exchange(other_.m_capacity, 0)}
{
}

template<typename T>
inline Vector<T>::~Vector() {
  clear();
  allocator().deallocate(m_data);
}

template<typename T>
inline Vector<T>& Vector<T>::operator=(const Vector& _other) {
  RX_ASSERT(&_other != this, "self assignment");

  clear();
  allocator().deallocate(m_data);

  m_size = _other.m_size;
  m_capacity = _other.m_capacity;

  m_data = reinterpret_cast<T*>(allocator().allocate(sizeof(T), _other.m_capacity));
  RX_ASSERT(m_data, "out of memory");

  if (m_size) {
    if constexpr(traits::is_trivially_copyable<T>) {
      detail::copy(m_data, _other.m_data, _other.m_size * sizeof *m_data);
    } else for (Size i = 0; i < m_size; i++) {
      Utility::construct<T>(m_data + i, _other.m_data[i]);
    }
  }

  return *this;
}

template<typename T>
inline Vector<T>& Vector<T>::operator=(Vector&& other_) {
  RX_ASSERT(&other_ != this, "self assignment");

  clear();
  allocator().deallocate(m_data);

  m_allocator = other_.m_allocator;
  m_data = Utility::exchange(other_.m_data, nullptr);
  m_size = Utility::exchange(other_.m_size, 0);
  m_capacity = Utility::exchange(other_.m_capacity, 0);

  return *this;
}

template<typename T>
inline T& Vector<T>::operator[](Size _index) {
  RX_ASSERT(m_data && in_range(_index), "out of bounds (%zu >= %zu)", _index, m_size);
  return m_data[_index];
}

template<typename T>
inline const T& Vector<T>::operator[](Size _index) const {
  RX_ASSERT(m_data && in_range(_index), "out of bounds (%zu >= %zu)", _index, m_size);
  return m_data[_index];
}

template<typename T>
bool Vector<T>::grow_or_shrink_to(Size _size) {
  if (!reserve(_size)) {
    return false;
  }

  if constexpr (!traits::is_trivially_destructible<T>) {
    for (Size i = m_size; i > _size; --i) {
      Utility::destruct<T>(m_data + (i - 1));
    }
  }

  return true;
}

template<typename T>
bool Vector<T>::resize(Size _size, const T& _value) {
  if (!grow_or_shrink_to(_size)) {
    return false;
  }

  // Copy construct the objects.
  for (Size i{m_size}; i < _size; i++) {
    if constexpr(traits::is_trivially_copyable<T>) {
      m_data[i] = _value;
    } else {
      Utility::construct<T>(m_data + i, _value);
    }
  }

  m_size = _size;
  return true;
}

template<typename T>
bool Vector<T>::resize(Size _size, Utility::UninitializedTag) {
  RX_ASSERT(traits::is_trivially_copyable<T>,
    "T isn't trivial, cannot leave uninitialized");

  if (!grow_or_shrink_to(_size)) {
    return false;
  }

  m_size = _size;
  return true;
}

template<typename T>
bool Vector<T>::reserve(Size _size) {
  if (_size <= m_capacity) {
    return true;
  }

  // Always resize capacity with the Golden ratio.
  while (m_capacity < _size) {
    m_capacity = ((m_capacity + 1) * 3) / 2;
  }

  if constexpr (traits::is_trivially_copyable<T>) {
    T* resize = reinterpret_cast<T*>(allocator().reallocate(m_data, m_capacity * sizeof *m_data));
    if (RX_HINT_UNLIKELY(!resize)) {
      return false;
    }
    m_data = resize;
    return true;
  } else {
    T* resize = reinterpret_cast<T*>(allocator().allocate(sizeof(T), m_capacity));
    if (RX_HINT_UNLIKELY(!resize)) {
      return false;
    }

    // Avoid the heavy indirect call through |m_allocator| for freeing nullptr.
    if (m_data) {
      for (Size i{0}; i < m_size; i++) {
        Utility::construct<T>(resize + i, Utility::move(*(m_data + i)));
        Utility::destruct<T>(m_data + i);
      }
      allocator().deallocate(m_data);
    }
    m_data = resize;
    return true;
  }

  RX_HINT_UNREACHABLE();
}

template<typename T>
bool Vector<T>::append(const Vector& _other) {
  const auto new_size = m_size + _other.m_size;

  // Slight optimization for trivially copyable |T|.
  if constexpr (traits::is_trivially_copyable<T>) {
    if (!resize(new_size)) {
      return false;
    }
    // Use fast copy.
    detail::copy(m_data + m_size, _other.m_data, sizeof(T) * _other.m_size);
  } else {
    if (!reserve(new_size)) {
      return false;
    }

    // Copy construct the objects.
    for (Size i = 0; i < _other.m_size; i++) {
      Utility::construct<T>(m_data + m_size + i, _other[i]);
    }

    m_size = new_size;
  }

  return true;
}

template<typename T>
inline void Vector<T>::clear() {
  if (m_size) {
    if constexpr (!traits::is_trivially_destructible<T>) {
      RX_ASSERT(m_data, "m_data == nullptr when m_size != 0");
      for (Size i = m_size - 1; i < m_size; i--) {
        Utility::destruct<T>(m_data + i);
      }
    }
  }
  m_size = 0;
}

template<typename T>
inline Size Vector<T>::find(const T& _value) const {
  for (Size i{0}; i < m_size; i++) {
    if (m_data[i] == _value) {
      return i;
    }
  }
  return k_npos;
}

template<typename T>
template<typename F>
inline Size Vector<T>::find_if(F&& _compare) const {
  for (Size i{0}; i < m_size; i++) {
    if (_compare(m_data[i])) {
      return i;
    }
  }
  return k_npos;
}

template<typename T>
inline bool Vector<T>::push_back(const T& _value) {
  if (!grow_or_shrink_to(m_size + 1)) {
    return false;
  }

  // Copy construct object.
  Utility::construct<T>(m_data + m_size, _value);

  m_size++;
  return true;
}

template<typename T>
inline bool Vector<T>::push_back(T&& value_) {
  if (!grow_or_shrink_to(m_size + 1)) {
    return false;
  }

  // Move construct object.
  Utility::construct<T>(m_data + m_size, Utility::forward<T>(value_));

  m_size++;
  return true;
}

template<typename T>
inline void Vector<T>::pop_back() {
  RX_ASSERT(m_size, "empty vector");
  grow_or_shrink_to(m_size - 1);
  m_size--;
}

template<typename T>
template<typename... Ts>
inline bool Vector<T>::emplace_back(Ts&&... _args) {
  if (!grow_or_shrink_to(m_size + 1)) {
    return false;
  }

  // Forward construct object.
  Utility::construct<T>(m_data + m_size, Utility::forward<Ts>(_args)...);

  m_size++;
  return true;
}

template<typename T>
RX_HINT_FORCE_INLINE Size Vector<T>::size() const {
  return m_size;
}

template<typename T>
RX_HINT_FORCE_INLINE Size Vector<T>::capacity() const {
  return m_capacity;
}

template<typename T>
RX_HINT_FORCE_INLINE bool Vector<T>::is_empty() const {
  return m_size == 0;
}

template<typename T>
RX_HINT_FORCE_INLINE bool Vector<T>::in_range(Size _index) const {
  return _index < m_size;
}

template<typename T>
template<typename F>
inline bool Vector<T>::each_fwd(F&& _func) {
  for (Size i{0}; i < m_size; i++) {
    if constexpr (traits::is_same<traits::return_type<F>, bool>) {
      if (!_func(m_data[i])) {
        return false;
      }
    } else {
      _func(m_data[i]);
    }
  }
  return true;
}

template<typename T>
template<typename F>
inline bool Vector<T>::each_fwd(F&& _func) const {
  for (Size i{0}; i < m_size; i++) {
    if constexpr (traits::is_same<traits::return_type<F>, bool>) {
      if (!_func(m_data[i])) {
        return false;
      }
    } else {
      _func(m_data[i]);
    }
  }
  return true;
}

template<typename T>
template<typename F>
inline bool Vector<T>::each_rev(F&& _func) {
  for (Size i{m_size - 1}; i < m_size; i--) {
    if constexpr (traits::is_same<traits::return_type<F>, bool>) {
      if (!_func(m_data[i])) {
        return false;
      }
    } else {
      _func(m_data[i]);
    }
  }
  return true;
}

template<typename T>
inline void Vector<T>::erase(Size _from, Size _to) {
  const Size range{_to - _from};
  T* begin{m_data};
  T* end{m_data + m_size};
  T* first{begin + _from};
  T* last{begin + _to};

  for (T* value{last}, *dest{first}; value != end; ++value, ++dest) {
    *dest = Utility::move(*value);
  }

  if constexpr (!traits::is_trivially_destructible<T>) {
    for (T* value{end-range}; value < end; ++value) {
      Utility::destruct<T>(value);
    }
  }

  m_size -= range;
}

template<typename T>
template<typename F>
inline bool Vector<T>::each_rev(F&& _func) const {
  for (Size i{m_size - 1}; i < m_size; i--) {
    if constexpr (traits::is_same<traits::return_type<F>, bool>) {
      if (!_func(m_data[i])) {
        return false;
      }
    } else {
      _func(m_data[i]);
    }
  }
  return true;
}

template<typename T>
RX_HINT_FORCE_INLINE const T& Vector<T>::first() const {
  RX_ASSERT(m_data, "empty vector");
  return m_data[0];
}

template<typename T>
RX_HINT_FORCE_INLINE T& Vector<T>::first() {
  RX_ASSERT(m_data, "empty vector");
  return m_data[0];
}

template<typename T>
RX_HINT_FORCE_INLINE const T& Vector<T>::last() const {
  RX_ASSERT(m_data, "empty vector");
  return m_data[m_size - 1];
}

template<typename T>
RX_HINT_FORCE_INLINE T& Vector<T>::last() {
  RX_ASSERT(m_data, "empty vector");
  return m_data[m_size - 1];
}

template<typename T>
RX_HINT_FORCE_INLINE const T* Vector<T>::data() const {
  return m_data;
}

template<typename T>
RX_HINT_FORCE_INLINE T* Vector<T>::data() {
  return m_data;
}

template<typename T>
RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Vector<T>::allocator() const {
  return *m_allocator;
}

template<typename T>
inline Memory::View Vector<T>::disown() {
  Memory::View view{m_allocator, reinterpret_cast<Byte*>(data()), capacity() * sizeof(T)};
  m_data = nullptr;
  m_size = 0;
  m_capacity = 0;
  return view;
}

} // namespace rx

#endif // RX_CORE_VECTOR_H
