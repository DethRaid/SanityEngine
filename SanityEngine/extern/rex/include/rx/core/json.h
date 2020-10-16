#ifndef RX_CORE_JSON_H
#define RX_CORE_JSON_H
#include "rx/core/concurrency/atomic.h"

#include "rx/core/traits/return_type.h"
#include "rx/core/traits/is_same.h"
#include "rx/core/traits/detect.h"

#include "rx/core/utility/declval.h"
#include "rx/core/utility/exchange.h"

#include "rx/core/string.h"
#include "rx/core/optional.h"

#include "lib/json.h"

namespace Rx {

// 32-bit: 8 bytes
// 64-bit: 16 bytes
struct RX_API JSON {
  constexpr JSON();
  JSON(Memory::Allocator& _allocator, const char* _contents, Size _length);
  JSON(Memory::Allocator& _allocator, const char* _contents);
  JSON(Memory::Allocator& _allocator, const String& _contents);
  JSON(const char* _contents, Size _length);
  JSON(const char* _contents);
  JSON(const String& _contents);
  JSON(const JSON& _json);
  JSON(JSON&& json_);
  ~JSON();

  JSON& operator=(const JSON& _json);
  JSON& operator=(JSON&& json_);

  enum class Type {
    k_array,
    k_boolean,
    k_null,
    k_number,
    k_object,
    k_string,
    k_integer
  };

  operator bool() const;
  Optional<String> error() const;

  bool is_type(Type _type) const;

  bool is_array() const;
  bool is_array_of(Type _type) const;
  bool is_array_of(Type _type, Size _size) const;
  bool is_boolean() const;
  bool is_null() const;
  bool is_number() const;
  bool is_object() const;
  bool is_string() const;
  bool is_integer() const;

  JSON operator[](Size _index) const;
  bool as_boolean() const;
  Float64 as_number() const;
  Float32 as_float() const;
  Sint32 as_integer() const;
  JSON operator[](const char* _name) const;
  String as_string() const;
  String as_string_with_allocator(Memory::Allocator& _allocator) const;

  template<typename T>
  T decode(const T& _default) const;

  // # of elements for objects and arrays only
  Size size() const;
  bool is_empty() const;

  template<typename F>
  bool each(F&& _function) const;

  constexpr Memory::Allocator& allocator() const;

private:
  template<typename T>
  using HasFromJSON =
    decltype(Utility::declval<T>().from_json(Utility::declval<JSON>()));

  struct Shared {
    Shared(Memory::Allocator& _allocator, const char* _contents, Size _length);
    ~Shared();

    Shared* acquire();
    void release();

    Memory::Allocator& m_allocator;
    struct json_parse_result_s m_error;
    struct json_value_s* m_root;
    Concurrency::Atomic<Size> m_count;
  };

  JSON(Shared* _shared, struct json_value_s* _head);

  Shared* m_shared;
  struct json_value_s* m_value;
};

inline constexpr JSON::JSON()
  : m_shared{nullptr}
  , m_value{nullptr}
{
}

inline JSON::JSON(Memory::Allocator& _allocator, const String& _contents)
  : JSON{_allocator, _contents.data(), _contents.size()}
{
}

inline JSON::JSON(const char* _contents, Size _length)
  : JSON{Memory::SystemAllocator::instance(), _contents, _length}
{
}

inline JSON::JSON(const String& _contents)
  : JSON{Memory::SystemAllocator::instance(), _contents.data(), _contents.size()}
{
}

inline JSON::JSON(const JSON& _json)
  : m_shared{_json.m_shared->acquire()}
  , m_value{_json.m_value}
{
}

inline JSON::JSON(JSON&& json_)
  : m_shared{Utility::exchange(json_.m_shared, nullptr)}
  , m_value{Utility::exchange(json_.m_value, nullptr)}
{
}

inline JSON::~JSON() {
  if (m_shared) {
    m_shared->release();
  }
}

inline JSON& JSON::operator=(const JSON& _json) {
  RX_ASSERT(&_json != this, "self assignment");

  if (m_shared) {
    m_shared->release();
  }

  m_shared = _json.m_shared->acquire();
  m_value = _json.m_value;

  return *this;
}

inline JSON& JSON::operator=(JSON&& json_) {
  RX_ASSERT(&json_ != this, "self assignment");

  m_shared = Utility::exchange(json_.m_shared, nullptr);
  m_value = Utility::exchange(json_.m_value, nullptr);

  return *this;
}

inline JSON::operator bool() const {
  return m_shared && m_shared->m_root;
}

inline bool JSON::is_array() const {
  return is_type(Type::k_array);
}

inline bool JSON::is_array_of(Type _type) const {
  if (!is_array()) {
    return false;
  }

  return each([_type](const JSON& _value) {
    return _value.is_type(_type);
  });
}

inline bool JSON::is_array_of(Type _type, Size _size) const {
  if (!is_array()) {
    return false;
  }

  if (size() != _size) {
    return false;
  }

  return each([_type](const JSON& _value) {
    return _value.is_type(_type);
  });
}

inline bool JSON::is_boolean() const {
  return is_type(Type::k_boolean);
}

inline bool JSON::is_null() const {
  return is_type(Type::k_null);
}

inline bool JSON::is_number() const {
  return is_type(Type::k_number);
}

inline bool JSON::is_object() const {
  return is_type(Type::k_object);
}

inline bool JSON::is_string() const {
  return is_type(Type::k_string);
}

inline bool JSON::is_integer() const {
  return is_type(Type::k_integer);
}

inline bool JSON::is_empty() const {
  return size() == 0;
}

inline String JSON::as_string() const {
  return as_string_with_allocator(Memory::SystemAllocator::instance());
}

template<typename T>
inline T JSON::decode(const T& _default) const {
  if constexpr(traits::is_same<T, Float32> || traits::is_same<T, Float64>) {
    if (is_number()) {
      return as_number();
    }
  } else if constexpr(traits::is_same<T, Sint32>) {
    if (is_integer()) {
      return as_integer();
    }
  } else if constexpr(traits::is_same<T, String>) {
    if (is_string()) {
      return as_string();
    }
  } else if constexpr(traits::detect<T, HasFromJSON>) {
    return T::from_json(*this);
  }

  return _default;
}

template<typename F>
inline bool JSON::each(F&& _function) const {
  const bool array = is_array();
  const bool object = is_object();

  RX_ASSERT(array || object, "not enumerable");

  if (array) {
    auto array = reinterpret_cast<struct json_array_s*>(m_value->payload);
    for (auto element = array->start; element; element = element->next) {
      if constexpr(traits::is_same<traits::return_type<F>, bool>) {
        if (!_function({m_shared, element->value})) {
          return false;
        }
      } else {
        _function({m_shared, element->value});
      }
    }
  } else {
    auto object = reinterpret_cast<struct json_object_s*>(m_value->payload);
    for (auto element = object->start; element; element = element->next) {
      if constexpr(traits::is_same<traits::return_type<F>, bool>) {
        if (!_function({m_shared, element->value})) {
          return false;
        }
      } else {
        _function({m_shared, element->value});
      }
    }
  }

  return true;
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& JSON::allocator() const {
  RX_ASSERT(m_shared, "reference count reached zero");
  return m_shared->m_allocator;
}

} // namespace rx

#endif // RX_CORE_JSON_H
