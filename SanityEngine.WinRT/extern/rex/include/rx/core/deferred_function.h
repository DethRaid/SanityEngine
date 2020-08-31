#ifndef RX_CORE_DEFERRED_FUNCTION_H
#define RX_CORE_DEFERRED_FUNCTION_H
#include "rx/core/function.h"

namespace Rx {

// callable function that gets called when the object goes out of scope
template<typename T>
struct DeferredFunction {
  template<typename F>
  constexpr DeferredFunction(Memory::Allocator& _allocator, F&& _function);

  template<typename F>
  constexpr DeferredFunction(F&& _function);

  ~DeferredFunction();

  constexpr Memory::Allocator& allocator() const;

private:
  Function<T> m_function;
};

template<typename T>
template<typename F>
inline constexpr DeferredFunction<T>::DeferredFunction(Memory::Allocator& _allocator, F&& _function)
  : m_function{_allocator, Utility::forward<F>(_function)}
{
}

template<typename T>
template<typename F>
inline constexpr DeferredFunction<T>::DeferredFunction(F&& _function)
  : DeferredFunction{Memory::SystemAllocator::instance(), Utility::forward<F>(_function)}
{
}

template<typename T>
inline DeferredFunction<T>::~DeferredFunction() {
  m_function();
}

template<typename T>
RX_HINT_FORCE_INLINE constexpr Memory::Allocator& DeferredFunction<T>::allocator() const {
  return m_function.allocator();
}

} // namespace rx

#endif // RX_CORE_DEFERRED_FUNCTION_H
