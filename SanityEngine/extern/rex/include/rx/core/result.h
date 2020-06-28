#ifndef RX_CORE_RESULT_H
#define RX_CORE_RESULT_H
#include "rx/core/string.h"
#include "rx/core/utility/nat.h"

namespace Rx {

struct Error {
  RX_MARK_NO_COPY(Error);

  template<typename... Ts>
  Error(Memory::Allocator& _allocator, const char* _format, Ts&&... _arguments);
  Error(Memory::Allocator& _allocator, const char* _error);
  Error(Memory::Allocator& _allocator, const String& _error);
  Error(String& error_);

  template<typename... Ts>
  Error(const char* _format, Ts&&... _arguments);
  Error(const char* _error);
  Error(const String& _error);

  Error(Error&& error_);

  Error& operator=(Error& error_);

private:
  template<typename>
  friend struct Result;

  String m_error;
};

template<typename T>
struct Result {
  constexpr Result();
  constexpr Result(const T& _value);
  Result(T&& value_);
  Result(Error&& error_);
  ~Result();

  constexpr bool is_valid() const;
  constexpr operator bool() const;

  const String& error() const;
  String&& error();

  const T& value() const;
  T&& value();

  const T& operator*() const &;
  const T* operator->() const;

private:
  union {
    Utility::Nat as_nat;
    T as_value;
    Error as_error;
  };

  enum {
    UNINIT,
    VALUE,
    ERROR
  } m_state;
};

// Error
template<typename... Ts>
inline Error::Error(Memory::Allocator& _allocator, const char* _format, Ts&&... _arguments)
  : m_error{String::format(_allocator, _format, Utility::forward<Ts>(_arguments)...)}
{
}

inline Error::Error(Memory::Allocator& _allocator, const char* _error)
  : m_error{_allocator, _error}
{
}

inline Error::Error(Memory::Allocator& _allocator, const String& _error)
  : m_error{_allocator, _error}
{
}

template<typename... Ts>
inline Error::Error(const char* _format, Ts&&... _arguments)
  : Error{Memory::SystemAllocator::instance(), _format, Utility::forward<Ts>(_arguments)...}
{
}

inline Error::Error(const char* _error)
  : Error{Memory::SystemAllocator::instance(), _error}
{
}

inline Error::Error(const String& _error)
  : Error{Memory::SystemAllocator::instance(), _error}
{
}

inline Error::Error(Error&& error_)
  : m_error{Utility::move(error_.m_error)}
{
}

inline Error& Error::operator=(Error& error_) {
  m_error = Utility::move(error_.m_error);
  return *this;
}

// Result
template<typename T>
inline constexpr Result<T>::Result()
  : as_nat{}
  , m_state{UNINIT}
{
}

template<typename T>
inline constexpr Result<T>::Result(const T& _value)
  : as_value{_value}
  , m_state{VALUE}
{
}

template<typename T>
inline Result<T>::Result(T&& value_)
  : as_value{Utility::move(value_)}
  , m_state{VALUE}
{
}

template<typename T>
inline Result<T>::Result(Error&& error_)
  : as_error{Utility::move(error_)}
  , m_state{ERROR}
{
}

template<typename T>
inline Result<T>::~Result() {
  switch (m_state) {
  case ERROR:
    Utility::destruct<Error>(&as_error);
    break;
  case VALUE:
    Utility::destruct<T>(&as_value);
    break;
  case UNINIT:
    break;
  }
}

template<typename T>
inline constexpr bool Result<T>::is_valid() const {
  return m_state == VALUE;
}

template<typename T>
inline constexpr Result<T>::operator bool() const {
  return is_valid();
}

template<typename T>
inline const String& Result<T>::error() const {
  RX_ASSERT(m_state == ERROR, "not an error");
  return as_error.m_error;
}

template<typename T>
inline String&& Result<T>::error() {
  RX_ASSERT(m_state == ERROR, "not an error");
  m_state = UNINIT;
  return Utility::move(as_error.m_error);
}

template<typename T>
inline const T& Result<T>::value() const {
  RX_ASSERT(m_state == VALUE, "no value");
  return as_value;
}

template<typename T>
inline T&& Result<T>::value() {
  RX_ASSERT(m_state == VALUE, "no value");
  m_state = UNINIT;
  return Utility::move(as_value);
}

template<typename T>
inline const T& Result<T>::operator*() const & {
  return value();
}

template<typename T>
inline const T* Result<T>::operator->() const {
  return &value();
}

} // namespace Rx

#endif // RX_CORE_RESULT_H
