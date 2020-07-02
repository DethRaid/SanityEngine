#ifndef RX_CORE_FUNCTION_H
#define RX_CORE_FUNCTION_H
#include "rx/core/traits/is_callable.h"
#include "rx/core/traits/enable_if.h"

#include "rx/core/utility/exchange.h"

#include "rx/core/memory/system_allocator.h"

namespace Rx {

// 32-bit: 12 bytes
// 64-bit: 24 bytes
template<typename T>
struct Function;

template<typename R, typename... Ts>
struct Function<R(Ts...)> {
  constexpr Function(Memory::Allocator& _allocator);
  constexpr Function();

  template<typename F, typename = traits::enable_if<traits::is_callable<F, Ts...>>>
  Function(Memory::Allocator& _allocator, F&& _function);

  template<typename F, typename = traits::enable_if<traits::is_callable<F, Ts...>>>
  Function(F&& _function);

  Function(const Function& _function);
  Function(Function&& function_);

  Function& operator=(const Function& _function);
  Function& operator=(Function&& function_);
  Function& operator=(NullPointer);

  ~Function();

  R operator()(Ts... _arguments) const;

  operator bool() const;

  constexpr Memory::Allocator& allocator() const;

private:
  enum class Lifetime {
    k_construct,
    k_destruct
  };

  using InvokeFn = R (*)(const Byte*, Ts&&...);
  using ModifyLifetimeFn = void (*)(Lifetime, Byte*, const Byte*);

  template<typename F>
  static R invoke(const Byte* _function, Ts&&... _arguments) {
    if constexpr(traits::is_same<R, void>) {
      (*reinterpret_cast<const F*>(_function))(Utility::forward<Ts>(_arguments)...);
    } else {
      return (*reinterpret_cast<const F*>(_function))(Utility::forward<Ts>(_arguments)...);
    }
  }

  template<typename F>
  static void modify_lifetime(Lifetime _lifetime, Byte* _dst, const Byte* _src) {
    switch (_lifetime) {
    case Lifetime::k_construct:
      Utility::construct<F>(_dst, *reinterpret_cast<const F*>(_src));
      break;
    case Lifetime::k_destruct:
      Utility::destruct<F>(_dst);
      break;
    }
  }

  // Keep control block aligned so the function proceeding it in |m_data| is
  // always aligned by |k_alignment| too.
  struct alignas(Memory::Allocator::k_alignment) ControlBlock {
    constexpr ControlBlock(ModifyLifetimeFn _modify_lifetime, InvokeFn _invoke)
      : modify_lifetime{_modify_lifetime}
      , invoke{_invoke}
    {
    }
    ModifyLifetimeFn modify_lifetime;
    InvokeFn invoke;
  };

  void destroy();

  ControlBlock* control();
  const ControlBlock* control() const;

  Byte* storage();
  const Byte* storage() const;

  Memory::Allocator* m_allocator;
  Byte* m_data;
  Size m_size;
};

template<typename R, typename... Ts>
inline constexpr Function<R(Ts...)>::Function(Memory::Allocator& _allocator)
  : m_allocator{&_allocator}
  , m_data{nullptr}
  , m_size{0}
{
}

template<typename R, typename... Ts>
inline constexpr Function<R(Ts...)>::Function()
  : Function{Memory::SystemAllocator::instance()}
{
}

template<typename R, typename... Ts>
template<typename F, typename>
inline Function<R(Ts...)>::Function(F&& _function)
  : Function{Memory::SystemAllocator::instance(), Utility::forward<F>(_function)}
{
}

template<typename R, typename... Ts>
template<typename F, typename>
inline Function<R(Ts...)>::Function(Memory::Allocator& _allocator, F&& _function)
  : Function{_allocator}
{
  m_size = sizeof(ControlBlock) + sizeof _function;
  m_data = allocator().allocate(m_size);
  RX_ASSERT(m_data, "out of memory");

  Utility::construct<ControlBlock>(m_data, &modify_lifetime<F>, &invoke<F>);
  Utility::construct<F>(storage(), Utility::forward<F>(_function));
}

template<typename R, typename... Ts>
inline Function<R(Ts...)>::Function(const Function& _function)
  : Function{_function.allocator()}
{
  if (_function.m_data) {
    m_size = _function.m_size;
    m_data = allocator().allocate(m_size);
    RX_ASSERT(m_data, "out of memory");

    // Copy construct the control block and the function.
    Utility::construct<ControlBlock>(m_data, *_function.control());
    control()->modify_lifetime(Lifetime::k_construct, storage(), _function.storage());
  }
}

template<typename R, typename... Ts>
inline Function<R(Ts...)>::Function(Function&& function_)
  : m_allocator{function_.m_allocator}
  , m_data{Utility::exchange(function_.m_data, nullptr)}
  , m_size{Utility::exchange(function_.m_size, 0)}
{
}

template<typename R, typename... Ts>
inline Function<R(Ts...)>& Function<R(Ts...)>::operator=(const Function& _function) {
  RX_ASSERT(&_function != this, "self assignment");

  if (m_data) {
    control()->modify_lifetime(Lifetime::k_destruct, storage(), nullptr);
  }

  if (_function.m_data) {
    // Reallocate storage to make function fit.
    if (_function.m_size > m_size) {
      m_size = _function.m_size;
      m_data = allocator().reallocate(m_data, m_size);
      RX_ASSERT(m_data, "out of memory");
    }
    // Copy construct the control block and the function.
    Utility::construct<ControlBlock>(m_data, *_function.control());
    control()->modify_lifetime(Lifetime::k_construct, storage(), _function.storage());
  } else {
    destroy();
  }

  return *this;
}

template<typename R, typename... Ts>
inline Function<R(Ts...)>& Function<R(Ts...)>::operator=(Function&& function_) {
  RX_ASSERT(&function_ != this, "self assignment");

  if (m_data) {
    control()->modify_lifetime(Lifetime::k_destruct, storage(), nullptr);
  }

  m_allocator = function_.m_allocator;
  m_size = Utility::exchange(function_.m_size, 0);
  m_data = Utility::exchange(function_.m_data, nullptr);

  return *this;
}

template<typename R, typename... Ts>
inline Function<R(Ts...)>& Function<R(Ts...)>::operator=(NullPointer) {
  if (m_data) {
    control()->modify_lifetime(Lifetime::k_destruct, storage(), nullptr);
    destroy();
  }
  return *this;
}

template<typename R, typename... Ts>
inline Function<R(Ts...)>::~Function() {
  if (m_data) {
    control()->modify_lifetime(Lifetime::k_destruct, storage(), nullptr);
    destroy();
  }
}

template<typename R, typename... Ts>
inline R Function<R(Ts...)>::operator()(Ts... _arguments) const {
  if constexpr(traits::is_same<R, void>) {
    control()->invoke(storage(), Utility::forward<Ts>(_arguments)...);
  } else {
    return control()->invoke(storage(), Utility::forward<Ts>(_arguments)...);
  }
}

template<typename R, typename... Ts>
Function<R(Ts...)>::operator bool() const {
  return m_size != 0;
}

template<typename R, typename... Ts>
RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Function<R(Ts...)>::allocator() const {
  return *m_allocator;
}

template<typename R, typename... Ts>
inline void Function<R(Ts...)>::destroy() {
  allocator().deallocate(m_data);
  m_data = nullptr;
  m_size = 0;
}

template<typename R, typename... Ts>
RX_HINT_FORCE_INLINE typename Function<R(Ts...)>::ControlBlock* Function<R(Ts...)>::control() {
  return reinterpret_cast<ControlBlock*>(m_data);
}

template<typename R, typename... Ts>
RX_HINT_FORCE_INLINE const typename Function<R(Ts...)>::ControlBlock* Function<R(Ts...)>::control() const {
  return reinterpret_cast<const ControlBlock*>(m_data);
}

template<typename R, typename... Ts>
RX_HINT_FORCE_INLINE Byte* Function<R(Ts...)>::storage() {
  return m_data + sizeof(ControlBlock);
}

template<typename R, typename... Ts>
RX_HINT_FORCE_INLINE const Byte* Function<R(Ts...)>::storage() const {
  return m_data + sizeof(ControlBlock);
}

} // namespace rx::core

#endif // RX_CORE_FUNCTION_H
