#ifndef RX_CORE_OPTIONAL_H
#define RX_CORE_OPTIONAL_H
#include "rx/core/assert.h" // RX_ASSERT
#include "rx/core/utility/move.h"
#include "rx/core/uninitialized.h"

namespace Rx {

constexpr const struct {} nullopt;

template<typename T>
struct Optional {
  constexpr Optional(decltype(nullopt));
  constexpr Optional();
  constexpr Optional(T&& data_);
  constexpr Optional(const T& _data);
  constexpr Optional(Optional&& other_);
  constexpr Optional(const Optional& _other);

  ~Optional();

  Optional& operator=(T&& data_);
  Optional& operator=(const T& _data);
  Optional& operator=(Optional&& other_);
  Optional& operator=(const Optional& _other);

  operator bool() const;

  bool has_value() const;

  T& operator*();
  const T& operator*() const;
  T* operator->();
  const T* operator->() const;

private:
  Uninitialized<T> m_data;
  bool m_init;
};

template<typename T>
inline constexpr Optional<T>::Optional(decltype(nullopt))
  : m_data{}
  , m_init{false}
{
}

template<typename T>
inline constexpr Optional<T>::Optional()
  : m_data{}
  , m_init{false}
{
}

template<typename T>
inline constexpr Optional<T>::Optional(T&& data_)
  : m_data{}
  , m_init{true}
{
  m_data.init(Utility::forward<T>(data_));
}

template<typename T>
inline constexpr Optional<T>::Optional(const T& _data)
  : m_data{}
  , m_init{true}
{
  m_data.init(_data);
}

template<typename T>
inline constexpr Optional<T>::Optional(Optional&& other_)
  : m_data{}
  , m_init{other_.m_init}
{
  if (m_init) {
    auto& data{other_.m_data};
    m_data.init(Utility::move(*data.data()));
    data.fini();
  }
  other_.m_init = false;
}

template<typename T>
inline constexpr Optional<T>::Optional(const Optional& _other)
  : m_data{}
  , m_init{_other.m_init}
{
  if (m_init) {
    const auto& data{_other.m_data};
    m_data.init(*data.data());
  }
}

template<typename T>
inline Optional<T>& Optional<T>::operator=(T&& data_) {
  if (m_init) {
    m_data.fini();
  }
  m_init = true;
  m_data.init(Utility::forward<T>(data_));
  return *this;
}

template<typename T>
inline Optional<T>& Optional<T>::operator=(const T& _data) {
  if (m_init) {
    m_data.fini();
  }
  m_init = true;
  m_data.init(_data);
  return *this;
}

template<typename T>
inline Optional<T>& Optional<T>::operator=(Optional&& other_) {
  RX_ASSERT(&other_ != this, "self assignment");

  if (m_init) {
    m_data.fini();
  }

  m_init = other_.m_init;

  if (m_init) {
    auto& data{other_.m_data};
    m_data.init(Utility::move(*data.data()));
    data.fini();
  }

  other_.m_init = false;

  return *this;
}

template<typename T>
inline Optional<T>& Optional<T>::operator=(const Optional& _other) {
  RX_ASSERT(&_other != this, "self assignment");

  if (m_init) {
    m_data.fini();
  }

  m_init = _other.m_init;

  if (m_init) {
    const auto& data{_other.m_data};
    m_data.init(*data.data());
  }

  return *this;
}

template<typename T>
inline Optional<T>::~Optional() {
  if (m_init) {
    m_data.fini();
  }
}

template<typename T>
inline Optional<T>::operator bool() const {
  return m_init;
}

template<typename T>
inline bool Optional<T>::has_value() const {
  return m_init;
}

template<typename T>
inline T& Optional<T>::operator*() {
  RX_ASSERT(m_init, "not valid");
  return *m_data.data();
}

template<typename T>
inline const T& Optional<T>::operator*() const {
  RX_ASSERT(m_init, "not valid");
  return *m_data.data();
}

template<typename T>
inline T* Optional<T>::operator->() {
  RX_ASSERT(m_init, "not valid");
  return m_data.data();
}

template<typename T>
inline const T* Optional<T>::operator->() const {
  RX_ASSERT(m_init, "not valid");
  return m_data.data();
}

} // namespace rx

#endif // RX_CORE_OPTIONAL_H
