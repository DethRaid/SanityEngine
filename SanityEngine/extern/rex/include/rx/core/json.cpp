#include <stdlib.h> // strtod
#include <string.h> // strcmp

#include "rx/core/json.h"
#include "rx/core/hints/unreachable.h"

#include "rx/core/math/floor.h"

namespace Rx {

static const char* json_parse_error_to_string(enum json_parse_error_e _error) {
  switch (_error) {
  case json_parse_error_expected_comma_or_closing_bracket:
    return "expected either a comma or closing '}' or ']'";
  case json_parse_error_expected_colon:
    return "expected a colon";
  case json_parse_error_expected_opening_quote:
    return "expected opening quote '\"'";
  case json_parse_error_invalid_string_escape_sequence:
    return "invalid string escape sequence";
  case json_parse_error_invalid_number_format:
    return "invalid number formatting";
  case json_parse_error_invalid_value:
    return "invalid value";
  case json_parse_error_premature_end_of_buffer:
    return "premature end of buffer";
  case json_parse_error_invalid_string:
    return "malformed string";
  case json_parse_error_allocator_failed:
    return "out of memory";
  case json_parse_error_unexpected_trailing_characters:
    return "unexpected trailing characters";
  case json_parse_error_unknown:
    [[fallthrough]];
  default:
    return "unknown error";
  }
}

static void* json_allocator(void* _user, Size _size) {
  auto allocator{reinterpret_cast<Memory::Allocator*>(_user)};
  return allocator->allocate(_size);
}

JSON::Shared::Shared(Memory::Allocator& _allocator, const char* _contents, Size _length)
  : m_allocator{_allocator}
  , m_root{nullptr}
  , m_count{1}
{
  m_root = json_parse_ex(_contents, _length,
    (json_parse_flags_allow_c_style_comments |
     json_parse_flags_allow_location_information |
     json_parse_flags_allow_unquoted_keys |
     json_parse_flags_allow_multi_line_strings),
    json_allocator,
    &m_allocator,
    &m_error);
}

JSON::Shared* JSON::Shared::acquire() {
  RX_ASSERT(m_count, "consistency check failed");
  m_count++;
  return this;
}

void JSON::Shared::release() {
  if (--m_count == 0) {
    m_allocator.destroy<Shared>(this);
  }
}

JSON::Shared::~Shared() {
  m_allocator.deallocate(m_root);
}

JSON::JSON(Memory::Allocator& _allocator, const char* _contents, Size _length)
  : m_shared{nullptr}
  , m_value{nullptr}
{
  // Construct the shared state.
  m_shared = _allocator.create<Shared>(_allocator, _contents, _length);
  RX_ASSERT(m_shared, "out of memory");

  // We hold a reference to the shared state already. Just take the root JSON
  // value as the base to begin.
  m_value = m_shared->m_root;
}

JSON::JSON(Memory::Allocator& _allocator, const char* _contents)
  : JSON{_allocator, _contents, strlen(_contents)}
{
}

JSON::JSON(const char* _contents)
  : JSON{Memory::SystemAllocator::instance(), _contents, strlen(_contents)}
{
}

JSON::JSON(Shared* _shared, struct json_value_s* _value)
  : m_shared{_shared->acquire()}
  , m_value{_value}
{
}

Optional<String> JSON::error() const {
  if (m_shared) {
    const auto code =
      static_cast<enum json_parse_error_e>(m_shared->m_error.error);
    return String::format("%zu:%zu %s", m_shared->m_error.error_line_no,
      m_shared->m_error.error_row_no, json_parse_error_to_string(code));
  }
  return nullopt;
}

bool JSON::is_type(Type _type) const {
  switch (_type) {
  case Type::k_array:
    return m_value->type == json_type_array;
  case Type::k_boolean:
    return m_value->type == json_type_true || m_value->type == json_type_false;
  case Type::k_integer:
    return m_value->type == json_type_number && Math::floor(as_number()) == as_number();
  case Type::k_null:
    return !m_value || m_value->type == json_type_null;
  case Type::k_number:
    return m_value->type == json_type_number;
  case Type::k_object:
    return m_value->type == json_type_object;
  case Type::k_string:
    return m_value->type == json_type_string;
  }

  RX_HINT_UNREACHABLE();
}

JSON JSON::operator[](Size _index) const {
  RX_ASSERT(is_array() || is_object(), "not an indexable Type");

  if (is_array()) {
    auto array{reinterpret_cast<struct json_array_s*>(m_value->payload)};
    auto element{array->start};
    RX_ASSERT(_index < array->length, "out of bounds");
    for (Size i{0}; i < _index; i++) {
      element = element->next;
    }
    return {m_shared, element->value};
  } else {
    auto object{reinterpret_cast<struct json_object_s*>(m_value->payload)};
    auto element{object->start};
    RX_ASSERT(_index < object->length, "out of bounds");
    for (Size i{0}; i < _index; i++) {
      element = element->next;
    }
    return {m_shared, element->value};
  }

  return {};
}

bool JSON::as_boolean() const {
  RX_ASSERT(is_boolean(), "not a boolean");
  return m_value->type == json_type_true;
}

Float64 JSON::as_number() const {
  RX_ASSERT(is_number(), "not a number");
  auto number{reinterpret_cast<struct json_number_s*>(m_value->payload)};
  return strtod(number->number, nullptr);
}

Float32 JSON::as_float() const {
  return static_cast<Float32>(as_number());
}

Sint32 JSON::as_integer() const {
  RX_ASSERT(is_integer(), "not a integer");
  return static_cast<Sint32>(as_number());
}

JSON JSON::operator[](const char* _name) const {
  RX_ASSERT(is_object(), "not a object");
  auto object{reinterpret_cast<struct json_object_s*>(m_value->payload)};
  for (auto element{object->start}; element; element = element->next) {
    if (!strcmp(element->name->string, _name)) {
      return {m_shared, element->value};
    }
  }
  return {};
}

String JSON::as_string_with_allocator(Memory::Allocator& _allocator) const {
  RX_ASSERT(is_string(), "not a string");
  auto string{reinterpret_cast<struct json_string_s*>(m_value->payload)};
  return {_allocator, string->string, string->string_size};
}

Size JSON::size() const {
  RX_ASSERT(is_array() || is_object(), "not an indexable Type");
  switch (m_value->type) {
  case json_type_array:
    return reinterpret_cast<struct json_array_s*>(m_value->payload)->length;
  case json_type_object:
    return reinterpret_cast<struct json_object_s*>(m_value->payload)->length;
  }
  return 0;
}

} // namespace rx
