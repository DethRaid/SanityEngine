#ifndef RX_CONSOLE_VARIABLE_H
#define RX_CONSOLE_VARIABLE_H
#include <limits.h> // INT_{MIN, MAX}

#include "rx/core/assert.h" // RX_ASSERT
#include "rx/core/string.h" // string
#include "rx/core/global.h" // global
#include "rx/core/event.h" // event

#include "rx/math/vec2.h" // vec2{f,i}
#include "rx/math/vec3.h" // vec3{f,i}
#include "rx/math/vec4.h" // vec4{f,i}

namespace Rx::Console {

using ::Rx::Math::Vec2;
using ::Rx::Math::Vec2f;
using ::Rx::Math::Vec2i;

using ::Rx::Math::Vec3;
using ::Rx::Math::Vec3f;
using ::Rx::Math::Vec3i;

using ::Rx::Math::Vec4;
using ::Rx::Math::Vec4f;
using ::Rx::Math::Vec4i;

template<typename T>
struct Variable;

enum class VariableType {
  k_boolean,
  k_string,
  k_int,
  k_float,
  k_vec4f,
  k_vec4i,
  k_vec3f,
  k_vec3i,
  k_vec2f,
  k_vec2i,
};

enum class VariableStatus {
  k_success,
  k_out_of_range,
  k_type_mismatch
};

// Type traits
template<VariableType T>
struct VariableTypeTrait {
  static constexpr const VariableType type = T;
};

template<typename T>
struct VariableTrait;

template<> struct VariableTrait<bool>   : VariableTypeTrait<VariableType::k_boolean> {};
template<> struct VariableTrait<String> : VariableTypeTrait<VariableType::k_string> {};
template<> struct VariableTrait<Sint32> : VariableTypeTrait<VariableType::k_int> {};
template<> struct VariableTrait<Float32> : VariableTypeTrait<VariableType::k_float> {};
template<> struct VariableTrait<Vec2f>  : VariableTypeTrait<VariableType::k_vec2f> {};
template<> struct VariableTrait<Vec2i>  : VariableTypeTrait<VariableType::k_vec2i> {};
template<> struct VariableTrait<Vec3f>  : VariableTypeTrait<VariableType::k_vec3f> {};
template<> struct VariableTrait<Vec3i>  : VariableTypeTrait<VariableType::k_vec3i> {};
template<> struct VariableTrait<Vec4f>  : VariableTypeTrait<VariableType::k_vec4f> {};
template<> struct VariableTrait<Vec4i>  : VariableTypeTrait<VariableType::k_vec4i> {};

static constexpr const Sint32 k_int_min = -INT_MAX - 1;
static constexpr const Sint32 k_int_max = INT_MAX;
static constexpr const Float32 k_float_min = -FLT_MAX;
static constexpr const Float32 k_float_max = FLT_MAX;

struct VariableReference {
  VariableReference() = default;
  VariableReference(const char* _name, const char* _description, void* _handle, VariableType _type);

  template<typename T>
  const Variable<T>* try_cast() const;
  template<typename T>
  Variable<T>* try_cast();

  template<typename T>
  const Variable<T>* cast() const;
  template<typename T>
  Variable<T>* cast();

  const char* description() const;
  const char* name() const;
  VariableType type() const;

  void reset();
  String print_current() const;
  String print_range() const;
  String print_initial() const;

  bool is_initial() const;

private:
  friend struct Interface;

  const char* m_name;
  const char* m_description;
  void* m_handle;
  VariableType m_type;
  VariableReference* m_next;
};

// variable_referece
template<typename T>
inline const Variable<T>* VariableReference::try_cast() const {
  return m_type == VariableTrait<T>::type ? cast<T>() : nullptr;
}

template<typename T>
inline Variable<T>* VariableReference::try_cast() {
  return m_type == VariableTrait<T>::type ? cast<T>() : nullptr;
}

template<typename T>
inline const Variable<T>* VariableReference::cast() const {
  RX_ASSERT(m_type == VariableTrait<T>::type, "invalid cast");
  return reinterpret_cast<const Variable<T>*>(m_handle);
}

template<typename T>
inline Variable<T>* VariableReference::cast() {
  RX_ASSERT(m_type == VariableTrait<T>::type, "invalid cast");
  return reinterpret_cast<Variable<T>*>(m_handle);
}

inline const char* VariableReference::description() const {
  return m_description;
}

inline const char* VariableReference::name() const {
  return m_name;
}

inline VariableType VariableReference::type() const {
  return m_type;
}

template<typename T>
struct Variable {
  using OnChangeEvent = Event<void(Variable<T>&)>;

  Variable(const char* _name, const char* _description, const T& _min,
    const T& _max, const T& _initial);

  operator const T&() const;
  const T& get() const;
  const T& min() const;
  const T& max() const;
  const T& initial() const;

  VariableReference* reference();
  const VariableReference* reference() const;

  void reset();
  VariableStatus set(const T& value);

  typename OnChangeEvent::Handle on_change(typename OnChangeEvent::Delegate&& _on_change);

private:
  VariableReference m_reference;
  T m_min;
  T m_max;
  T m_initial;
  T m_current;
  OnChangeEvent m_on_change;
};

// specialization for boolean
template<>
struct Variable<bool> {
  using OnChangeEvent = Event<void(Variable<bool>&)>;

  Variable(const char* _name, const char* _description, bool _initial);

  operator const bool&() const;
  const bool& get() const;
  const bool& initial() const;

  VariableReference* reference();
  const VariableReference* reference() const;

  void reset();
  VariableStatus set(bool value);

  void toggle();

  typename OnChangeEvent::Handle on_change(typename OnChangeEvent::Delegate&& _on_change);

private:
  VariableReference m_reference;
  bool m_initial;
  bool m_current;
  OnChangeEvent m_on_change;
};

// specialization for string
template<>
struct Variable<String> {
  using OnChangeEvent = Event<void(Variable<String>&)>;

  Variable(const char* _name, const char* _description, const char* _initial);

  operator const String&() const;
  const String& get() const;
  const char* initial() const;

  VariableReference* reference();
  const VariableReference* reference() const;

  void reset();
  VariableStatus set(const char* _value);
  VariableStatus set(const String& _value);

  typename OnChangeEvent::Handle on_change(typename OnChangeEvent::Delegate&& _on_change);

private:
  VariableReference m_reference;
  const char* m_initial;
  String m_current;
  OnChangeEvent m_on_change;
};

// specialization for vector types
template<typename T>
struct Variable<Vec2<T>> {
  using OnChangeEvent = Event<void(Variable<Vec2<T>>&)>;

  Variable(const char* _name, const char* _description, const Vec2<T>& _min,
           const Vec2<T>& _max, const Vec2<T>& _initial);

  operator const Vec2<T>&() const;
  const Vec2<T>& get() const;
  const Vec2<T>& min() const;
  const Vec2<T>& max() const;
  const Vec2<T>& initial() const;

  VariableReference* reference();
  const VariableReference* reference() const;

  void reset();
  VariableStatus set(const Vec2<T>& _value);

  typename OnChangeEvent::Handle on_change(typename OnChangeEvent::Delegate&& _on_change);

private:
  VariableReference m_reference;
  Vec2<T> m_min;
  Vec2<T> m_max;
  Vec2<T> m_initial;
  Vec2<T> m_current;
  OnChangeEvent m_on_change;
};

template<typename T>
struct Variable<Vec3<T>> {
  using OnChangeEvent = Event<void(Variable<Vec3<T>>&)>;

  Variable(const char* _name, const char* _description, const Vec3<T>& _min,
           const Vec3<T>& _max, const Vec3<T>& _initial);

  operator const Vec3<T>&() const;
  const Vec3<T>& get() const;
  const Vec3<T>& min() const;
  const Vec3<T>& max() const;
  const Vec3<T>& initial() const;

  VariableReference* reference();
  const VariableReference* reference() const;

  void reset();
  VariableStatus set(const Vec3<T>& value);

  typename OnChangeEvent::Handle on_change(typename OnChangeEvent::Delegate&& _on_change);

private:
  VariableReference m_reference;
  Vec3<T> m_min;
  Vec3<T> m_max;
  Vec3<T> m_initial;
  Vec3<T> m_current;
  OnChangeEvent m_on_change;
};

template<typename T>
struct Variable<Vec4<T>> {
  using OnChangeEvent = Event<void(Variable<Vec4<T>>&)>;

  Variable(const char* _name, const char* _description, const Vec4<T>& _min,
           const Vec4<T>& _max, const Vec4<T>& _initial);

  operator const Vec4<T>&() const;
  const Vec4<T>& get() const;
  const Vec4<T>& min() const;
  const Vec4<T>& max() const;
  const Vec4<T>& initial() const;

  VariableReference* reference();
  const VariableReference* reference() const;

  void reset();
  VariableStatus set(const Vec4<T>& value);

  typename OnChangeEvent::Handle on_change(typename OnChangeEvent::Delegate&& _on_change);

private:
  VariableReference m_reference;
  Vec4<T> m_min;
  Vec4<T> m_max;
  Vec4<T> m_initial;
  Vec4<T> m_current;
  OnChangeEvent m_on_change;
};

// Variable<T>
template<typename T>
inline Variable<T>::Variable(const char* _name, const char* _description,
  const T& _min, const T& _max, const T& _initial)
  : m_reference{_name, _description, static_cast<void*>(this), VariableTrait<T>::type}
  , m_min{_min}
  , m_max{_max}
  , m_initial{_initial}
  , m_current{_initial}
{
}

template<typename T>
inline Variable<T>::operator const T&() const {
  return m_current;
}

template<typename T>
inline const T& Variable<T>::get() const {
  return m_current;
}

template<typename T>
inline const T& Variable<T>::min() const {
  return m_min;
}

template<typename T>
inline const T& Variable<T>::max() const {
  return m_max;
}

template<typename T>
inline const T& Variable<T>::initial() const {
  return m_initial;
}

template<typename T>
inline const VariableReference* Variable<T>::reference() const {
  return &m_reference;
}

template<typename T>
inline VariableReference* Variable<T>::reference() {
  return &m_reference;
}

template<typename T>
inline void Variable<T>::reset() {
  m_current = m_initial;
}

template<typename T>
inline VariableStatus Variable<T>::set(const T& _value) {
  if (_value < m_min || _value > m_max) {
    return VariableStatus::k_out_of_range;
  }

  if (m_current != _value) {
    m_current = _value;
    m_on_change.signal(*this);
  }

  return VariableStatus::k_success;
}

template<typename T>
inline typename Variable<T>::OnChangeEvent::Handle Variable<T>::on_change(typename OnChangeEvent::Delegate&& on_change_) {
  return m_on_change.connect(Utility::move(on_change_));
}

// Variable<bool>
inline Variable<bool>::Variable(const char* _name, const char* _description, bool _initial)
  : m_reference{_name, _description, static_cast<void*>(this), VariableTrait<bool>::type}
  , m_initial{_initial}
  , m_current{_initial}
{
}

inline Variable<bool>::operator const bool&() const {
  return m_current;
}

inline const bool& Variable<bool>::get() const {
  return m_current;
}

inline const bool& Variable<bool>::initial() const {
  return m_initial;
}

inline VariableReference* Variable<bool>::reference() {
  return &m_reference;
}

inline const VariableReference* Variable<bool>::reference() const {
  return &m_reference;
}

inline void Variable<bool>::reset() {
  m_current = m_initial;
}

inline VariableStatus Variable<bool>::set(bool value) {
  if (m_current != value) {
    m_current = value;
    m_on_change.signal(*this);
  }
  return VariableStatus::k_success;
}

inline void Variable<bool>::toggle() {
  m_current = !m_current;
  m_on_change.signal(*this);
}

inline typename Variable<bool>::OnChangeEvent::Handle Variable<bool>::on_change(typename OnChangeEvent::Delegate&& on_change_) {
  return m_on_change.connect(Utility::move(on_change_));
}

// Variable<String>
inline Variable<String>::Variable(const char* _name, const char* _description, const char* _initial)
  : m_reference{_name, _description, static_cast<void*>(this), VariableTrait<String>::type}
  , m_initial{_initial}
  , m_current{_initial}
{
}

inline Variable<String>::operator const String&() const {
  return m_current;
}

inline const String& Variable<String>::get() const {
  return m_current;
}

inline const char* Variable<String>::initial() const {
  return m_initial;
}

inline VariableReference* Variable<String>::reference() {
  return &m_reference;
}

inline const VariableReference* Variable<String>::reference() const {
  return &m_reference;
}

inline void Variable<String>::reset() {
  m_current = m_initial;
}

inline VariableStatus Variable<String>::set(const char* _value) {
  if (m_current != _value) {
    m_current = _value;
    m_on_change.signal(*this);
  }
  return VariableStatus::k_success;
}

inline VariableStatus Variable<String>::set(const String& _value) {
  if (m_current != _value) {
    m_current = _value;
    m_on_change.signal(*this);
  }

  return VariableStatus::k_success;
}

inline typename Variable<String>::OnChangeEvent::Handle Variable<String>::on_change(typename OnChangeEvent::Delegate&& on_change_) {
  return m_on_change.connect(Utility::move(on_change_));
}

// Variable<vec2<T>>
template<typename T>
inline Variable<Vec2<T>>::Variable(const char* _name, const char* _description,
                                   const Vec2<T>& _min, const Vec2<T>& _max, const Vec2<T>& _initial)
  : m_reference{_name, _description, static_cast<void*>(this), VariableTrait<Vec2<T>>::type}
  , m_min{_min}
  , m_max{_max}
  , m_initial{_initial}
  , m_current{_initial}
{
}

template<typename T>
inline Variable<Vec2<T>>::operator const Vec2<T>&() const {
  return m_current;
}

template<typename T>
inline const Vec2<T>& Variable<Vec2<T>>::get() const {
  return m_current;
}

template<typename T>
inline const Vec2<T>& Variable<Vec2<T>>::min() const {
  return m_min;
}

template<typename T>
inline const Vec2<T>& Variable<Vec2<T>>::max() const {
  return m_max;
}

template<typename T>
inline const Vec2<T>& Variable<Vec2<T>>::initial() const {
  return m_initial;
}

template<typename T>
inline VariableReference* Variable<Vec2<T>>::reference() {
  return &m_reference;
}

template<typename T>
inline const VariableReference* Variable<Vec2<T>>::reference() const {
  return &m_reference;
}

template<typename T>
inline void Variable<Vec2<T>>::reset() {
  m_current = m_initial;
}

template<typename T>
inline VariableStatus Variable<Vec2<T>>::set(const Vec2<T>& _value) {
  if (_value.x < m_min.x || _value.y < m_min.y ||
      _value.x > m_max.x || _value.y > m_max.y)
  {
    return VariableStatus::k_out_of_range;
  }

  if (m_current != _value) {
    m_current = _value;
    m_on_change.signal(*this);
  }

  return VariableStatus::k_success;
}

template<typename T>
inline typename Variable<Vec2<T>>::OnChangeEvent::Handle Variable<Vec2<T>>::on_change(typename OnChangeEvent::Delegate&& on_change_) {
  return m_on_change.connect(Utility::move(on_change_));
}

// Variable<vec3<T>>
template<typename T>
inline Variable<Vec3<T>>::Variable(const char* _name, const char* _description,
                                   const Vec3<T>& _min, const Vec3<T>& _max, const Vec3<T>& _initial)
  : m_reference{_name, _description, static_cast<void*>(this), VariableTrait<Vec3<T>>::type}
  , m_min{_min}
  , m_max{_max}
  , m_initial{_initial}
  , m_current{_initial}
{
}

template<typename T>
inline Variable<Vec3<T>>::operator const Vec3<T>&() const {
  return m_current;
}

template<typename T>
inline const Vec3<T>& Variable<Vec3<T>>::get() const {
  return m_current;
}

template<typename T>
inline const Vec3<T>& Variable<Vec3<T>>::min() const {
  return m_min;
}

template<typename T>
inline const Vec3<T>& Variable<Vec3<T>>::max() const {
  return m_max;
}

template<typename T>
inline const Vec3<T>& Variable<Vec3<T>>::initial() const {
  return m_initial;
}

template<typename T>
inline VariableReference* Variable<Vec3<T>>::reference() {
  return &m_reference;
}

template<typename T>
inline const VariableReference* Variable<Vec3<T>>::reference() const {
  return &m_reference;
}

template<typename T>
inline void Variable<Vec3<T>>::reset() {
  m_current = m_initial;
}

template<typename T>
inline VariableStatus Variable<Vec3<T>>::set(const Vec3<T>& _value) {
  if (_value.x < m_min.x || _value.y < m_min.y || _value.z < m_min.z ||
      _value.x > m_max.x || _value.y > m_max.y || _value.z > m_max.z)
  {
    return VariableStatus::k_out_of_range;
  }
  if (m_current != _value) {
    m_current = _value;
    m_on_change.signal(*this);
  }
  return VariableStatus::k_success;
}

template<typename T>
inline typename Variable<Vec3<T>>::OnChangeEvent::Handle Variable<Vec3<T>>::on_change(typename OnChangeEvent::Delegate&& on_change_) {
  return m_on_change.connect(Utility::move(on_change_));
}

// Variable<vec4<T>>
template<typename T>
inline Variable<Vec4<T>>::Variable(const char* _name, const char* _description,
                                   const Vec4<T>& _min, const Vec4<T>& _max, const Vec4<T>& _initial)
  : m_reference{_name, _description, static_cast<void*>(this), VariableTrait<Vec4<T>>::type}
  , m_min{_min}
  , m_max{_max}
  , m_initial{_initial}
  , m_current{_initial}
{
}

template<typename T>
inline Variable<Vec4<T>>::operator const Vec4<T>&() const {
  return m_current;
}

template<typename T>
inline const Vec4<T>& Variable<Vec4<T>>::get() const {
  return m_current;
}

template<typename T>
inline const Vec4<T>& Variable<Vec4<T>>::min() const {
  return m_min;
}

template<typename T>
inline const Vec4<T>& Variable<Vec4<T>>::max() const {
  return m_max;
}

template<typename T>
inline const Vec4<T>& Variable<Vec4<T>>::initial() const {
  return m_initial;
}

template<typename T>
inline VariableReference* Variable<Vec4<T>>::reference() {
  return &m_reference;
}

template<typename T>
inline const VariableReference* Variable<Vec4<T>>::reference() const {
  return &m_reference;
}

template<typename T>
inline void Variable<Vec4<T>>::reset() {
  m_current = m_initial;
}

template<typename T>
inline VariableStatus Variable<Vec4<T>>::set(const Vec4<T>& _value) {
  if (_value.x < m_min.x || _value.y < m_min.y || _value.z < m_min.z || _value.w < m_min.w ||
      _value.x > m_max.x || _value.y > m_max.y || _value.z > m_max.z || _value.w > m_max.w)
  {
    return VariableStatus::k_out_of_range;
  }
  if (m_current != _value) {
    m_current = _value;
    m_on_change.signal(*this);
  }
  return VariableStatus::k_success;
}

template<typename T>
inline typename Variable<Vec4<T>>::OnChangeEvent::Handle Variable<Vec4<T>>::on_change(typename OnChangeEvent::Delegate&& on_change_) {
  return m_on_change.connect(Utility::move(on_change_));
}

const char* VariableType_as_string(VariableType _type);

inline bool VariableType_is_ranged(VariableType _type) {
  return _type != VariableType::k_boolean && _type != VariableType::k_string;
}

} // namespace rx::console

#define RX_CONSOLE_TRVAR(_type, _label, _name, _description, _min, _max, _initial) \
  static ::Rx::Global<::Rx::Console::Variable<_type>> _label \
    {"cvars", (_name), (_name), (_description), (_min), (_max), (_initial)}

#define RX_CONSOLE_TUVAR(_type, _label, _name, _description, _initial) \
  static ::Rx::Global<::Rx::Console::Variable<_type>> _label \
    {"cvars", (_name), (_name), (_description), (_initial)}

// helper macros to define console variables
#define RX_CONSOLE_BVAR(_label, _name, _description, _initial) \
  RX_CONSOLE_TUVAR(bool, _label, _name, (_description), (_initial))

#define RX_CONSOLE_SVAR(_label, _name, _description, _initial) \
  RX_CONSOLE_TUVAR(::Rx::String, _label, _name, (_description), (_initial))

#define RX_CONSOLE_IVAR(_label, _name, _description, _min, _max, _initial) \
  RX_CONSOLE_TRVAR(Sint32, _label, _name, (_description), (_min), (_max), (_initial))

#define RX_CONSOLE_FVAR(_label, _name, _description, _min, _max, _initial) \
  RX_CONSOLE_TRVAR(Float32, _label, _name, (_description), (_min), (_max), (_initial))

#define RX_CONSOLE_V2IVAR(_label, _name, _description, _min, _max, _initial) \
  RX_CONSOLE_TRVAR(::Rx::Math::Vec2i, _label, _name, (_description), (_min), (_max), (_initial))

#define RX_CONSOLE_V2FVAR(_label, _name, _description, _min, _max, _initial) \
  RX_CONSOLE_TRVAR(::Rx::Math::Vec2f, _label, _name, (_description), (_min), (_max), (_initial))

#define RX_CONSOLE_V3IVAR(_label, _name, _description, _min, _max, _initial) \
  RX_CONSOLE_TRVAR(::Rx::Math::Vec3i, _label, _name, (_description), (_min), (_max), (_initial))

#define RX_CONSOLE_V3FVAR(_label, _name, _description, _min, _max, _initial) \
  RX_CONSOLE_TRVAR(::Rx::Math::Vec3f, _label, _name, (_description), (_min), (_max), (_initial))

#define RX_CONSOLE_V4IVAR(_label, _name, _description, _min, _max, _initial) \
  RX_CONSOLE_TRVAR(::Rx::Math::Vec4i, _label, _name, (_description), (_min), (_max), (_initial))

#define RX_CONSOLE_V4FVAR(_label, _name, _description, _min, _max, _initial) \
  RX_CONSOLE_TRVAR(::Rx::Math::Vec4f, _label, _name, (_description), (_min), (_max), (_initial))

#endif // RX_CONSOLE_VARIABLE_H
