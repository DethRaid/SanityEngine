#ifndef RX_CORE_STRING_H
#define RX_CORE_STRING_H
#include "rx/core/assert.h" // RX_ASSERT
#include "rx/core/format.h" // format
#include "rx/core/vector.h" // vector

#include "rx/core/traits/remove_cvref.h"

#include "rx/core/memory/system_allocator.h" // memory::{system_allocator, allocator}

namespace Rx {

struct WideString;

// 32-bit: 16 + k_small_string bytes
// 64-bit: 32 + k_small_string bytes
struct String {
  static constexpr const Size k_npos{-1_z};
  static constexpr const Size k_small_string{16};

  constexpr String(Memory::Allocator& _allocator);
  String(Memory::Allocator& _allocator, const String& _contents);
  String(Memory::Allocator& _allocator, const char* _contents);
  String(Memory::Allocator& _allocator, const char* _contents, Size _size);
  String(Memory::Allocator& _allocator, const char* _first, const char* _last);

  constexpr String();
  String(const String& _contents);
  String(String&& contents_);
  String(const char* _contents);
  String(const char* _contents, Size _size);
  String(const char* _first, const char* _last);
  String(Memory::View _view);
  ~String();

  template<typename... Ts>
  static String format(Memory::Allocator& _allocator,
                       const char* _format, Ts&&... _arguments);

  template<typename... Ts>
  static String format(const char* _format, Ts&&... _arguments);

  String& operator=(const String& _contents);
  String& operator=(const char* _contents);
  String& operator=(String&& contents_);

  bool reserve(Size _size);
  bool resize(Size _size);

  Size find_first_of(int _ch) const;
  Size find_first_of(const char* _contents) const;
  Size find_first_of(const String& _contents) const;

  Size find_last_of(int _ch) const;
  Size find_last_of(const char* _contents) const;
  Size find_last_of(const String& _contents) const;

  Size size() const;
  Size capacity() const;

  bool is_empty() const;

  void clear();

  String& append(const char* _first, const char* _last);
  String& append(const char* _contents, Size _size);
  String& append(const char* _contents);
  String& append(const String& _contents);
  String& append(char _ch);

  bool insert_at(Size _position, const char* _contents, Size _size);
  bool insert_at(Size _position, const char* _contents);
  bool insert_at(Size _position, const String& _contents);

  // returns copy of string with leading characters in set removed
  String lstrip(const char* _set) const;

  // returns copy of string with trailing characters in set removed
  String rstrip(const char* _set) const;

  // returns copy of string with leading and trailing characters in set removed
  String strip(const char* _set) const;

  // split string by |token| up to |count| times, use |count| of zero for no limit
  Vector<String> split(Memory::Allocator& _allocator, int _ch, Size _count = 0) const;
  Vector<String> split(int _ch, Size _count = 0) const;

  // take substring from |offset| of |length|, use |length| of zero for whole string
  String substring(Size _offset, Size _length = 0) const;

  // scan string
  Size scan(const char* _scan_format, ...) const;

  char pop_back();

  void erase(Size _begin, Size _end);

  char& operator[](Size _index);
  const char& operator[](Size _index) const;

  char& first();
  const char& first() const;

  char& last();
  const char& last() const;

  char* data();
  const char* data() const;

  static String human_size_format(Size _size);

  bool begins_with(const char* _prefix) const;
  bool begins_with(const String& _prefix) const;

  bool ends_with(const char* _suffix) const;
  bool ends_with(const String& _suffix) const;

  bool contains(const char* _needle) const;
  bool contains(const String& _needle) const;

  Size hash() const;

  WideString to_utf16() const;

  constexpr Memory::Allocator& allocator() const;
  Memory::View disown();

private:
  static String formatter(Memory::Allocator& _allocator, const char* _format, ...);

  void swap(String& other);

  Ref<Memory::Allocator> m_allocator;
  char* m_data;
  char* m_last;
  char* m_capacity;

  char m_buffer[k_small_string];
};

// utf-16, Windows compatible "wide-string"
struct WideString {
  RX_MARK_NO_MOVE_ASSIGN(WideString);

  // custom allocator versions
  WideString(Memory::Allocator& _allocator);
  WideString(Memory::Allocator& _allocator, const WideString& _other);
  WideString(Memory::Allocator& _allocator, const Uint16* _contents);
  WideString(Memory::Allocator& _allocator, const Uint16* _contents, Size _size);

  // constructors that use system allocator
  WideString();
  WideString(const WideString& _other);
  WideString(const Uint16* _contents);
  WideString(const Uint16* _contents, Size _size);

  WideString(WideString&& other_);

  ~WideString();

  // disable all assignment operators because you're not supposed to use WideString
  // for any other purpose than to convert string (which is utf8) to utf16 for
  // interfaces expecting that, such as the ones on Windows
  WideString& operator=(const Uint16*) = delete;
  WideString& operator=(const char*) = delete;
  WideString& operator=(const String&) = delete;

  Size size() const;
  bool is_empty() const;

  Uint16& operator[](Size _index);
  const Uint16& operator[](Size _index) const;

  Uint16* data();
  const Uint16* data() const;

  bool resize(Size _size);

  String to_utf8() const;

  constexpr Memory::Allocator& allocator() const;

private:
  Ref<Memory::Allocator> m_allocator;

  Uint16* m_data;
  Size m_size;
};

// format function for string
template<>
struct FormatNormalize<String> {
  const char* operator()(const String& _value) const {
    return _value.data();
  }
};

// String
template<typename... Ts>
inline String String::format(Memory::Allocator& _allocator, const char* _format, Ts&&... _arguments) {
  return formatter(_allocator, _format, format_normalize(Utility::forward<Ts>(_arguments))...);
}

template<typename... Ts>
inline String String::format(const char* _format, Ts&&... _arguments) {
  return format(Memory::SystemAllocator::instance(), _format, Utility::forward<Ts>(_arguments)...);
}

inline String::String(Memory::Allocator& _allocator, const String& _contents)
  : String{_allocator, _contents.data(), _contents.size()}
{
}

inline constexpr String::String(Memory::Allocator& _allocator)
  : m_allocator{_allocator}
  , m_data{m_buffer}
  , m_last{m_buffer}
  , m_capacity{m_buffer + k_small_string}
  , m_buffer{}
{
  m_buffer[0] = '\0';
}

inline constexpr String::String()
  : String{Memory::SystemAllocator::instance()}
{
}

inline String::String(const String& _contents)
  : String{_contents.allocator(), _contents}
{
}

inline String::String(const char* _contents)
  : String{Memory::SystemAllocator::instance(), _contents}
{
}

inline String::String(const char* _contents, Size _size)
  : String{Memory::SystemAllocator::instance(), _contents, _size}
{
}

inline String::String(const char* _first, const char* _last)
  : String{Memory::SystemAllocator::instance(), _first, _last}
{
}

inline Size String::find_first_of(const String& _contents) const {
  return find_first_of(_contents.data());
}

inline Size String::find_last_of(const String& _contents) const {
  return find_last_of(_contents.data());
}

RX_HINT_FORCE_INLINE Size String::size() const {
  return m_last - m_data;
}

RX_HINT_FORCE_INLINE Size String::capacity() const {
  return m_capacity - m_data;
}

RX_HINT_FORCE_INLINE bool String::is_empty() const {
  return m_last - m_data == 0;
}

inline void String::clear() {
  (void)!!resize(0);
}

inline String& String::append(const char* contents, Size size) {
  return append(contents, contents + size);
}

inline String& String::append(const String& contents) {
  return append(contents.data(), contents.size());
}

inline String& String::append(char ch) {
  return append(&ch, 1);
}

inline bool String::insert_at(Size _position, const String& _contents) {
  return insert_at(_position, _contents.data(), _contents.size());
}

inline Vector<String> String::split(int _ch, Size _count) const {
  return split(allocator(), _ch, _count);
}

inline char& String::operator[](Size index) {
  // NOTE(dweiler): The <= is not a bug, indexing the null-terminator is allowed.
  RX_ASSERT(index <= size(), "out of bounds");
  return m_data[index];
}

inline const char& String::operator[](Size index) const {
  // NOTE(dweiler): The <= is not a bug, indexing the null-terminator is allowed.
  RX_ASSERT(index <= size(), "out of bounds");
  return m_data[index];
}

RX_HINT_FORCE_INLINE char& String::first() {
  return m_data[0];
}

RX_HINT_FORCE_INLINE const char& String::first() const {
  return m_data[0];
}

RX_HINT_FORCE_INLINE char& String::last() {
  RX_ASSERT(!is_empty(), "empty string");
  return m_data[size() - 1];
}

RX_HINT_FORCE_INLINE const char& String::last() const {
  RX_ASSERT(!is_empty(), "empty string");
  return m_data[size() - 1];
}

RX_HINT_FORCE_INLINE char* String::data() {
  return m_data;
}

RX_HINT_FORCE_INLINE const char* String::data() const {
  return m_data;
}

inline String operator+(const String& _lhs, const char* _rhs) {
  return String(_lhs).append(_rhs);
}

inline String operator+(const String& _lhs, const String& _rhs) {
  return String(_lhs).append(_rhs);
}

inline String operator+(const String& _lhs, const char _ch) {
  return String(_lhs).append(_ch);
}

inline String& operator+=(String& lhs_, const char* rhs) {
  lhs_.append(rhs);
  return lhs_;
}

inline String& operator+=(String& lhs_, char ch) {
  lhs_.append(ch);
  return lhs_;
}

inline String& operator+=(String& lhs_, const String& _contents) {
  lhs_.append(_contents);
  return lhs_;
}

// not inlined since it would explode code size
bool operator==(const String& lhs, const String& rhs);
bool operator!=(const String& lhs, const String& rhs);
bool operator<(const String& lhs, const String& rhs);
bool operator>(const String& lhs, const String& rhs);

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& String::allocator() const {
  return m_allocator;
}

// WideString
inline WideString::WideString()
  : WideString{Memory::SystemAllocator::instance()}
{
}

inline WideString::WideString(const Uint16* _contents)
  : WideString{Memory::SystemAllocator::instance(), _contents}
{
}

inline WideString::WideString(const Uint16* _contents, Size _size)
  : WideString{Memory::SystemAllocator::instance(), _contents, _size}
{
}

inline WideString::WideString(const WideString& _other)
  : WideString{_other.allocator(), _other}
{
}

RX_HINT_FORCE_INLINE Size WideString::size() const {
  return m_size;
}

RX_HINT_FORCE_INLINE bool WideString::is_empty() const {
  return m_size == 0;
}

inline Uint16& WideString::operator[](Size _index) {
  RX_ASSERT(_index <= m_size, "out of bounds");
  return m_data[_index];
}

inline const Uint16& WideString::operator[](Size _index) const {
  RX_ASSERT(_index <= m_size, "out of bounds");
  return m_data[_index];
}

RX_HINT_FORCE_INLINE Uint16* WideString::data() {
  return m_data;
}

RX_HINT_FORCE_INLINE const Uint16* WideString::data() const {
  return m_data;
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& WideString::allocator() const {
  return m_allocator;
}

Size utf16_to_utf8(const Uint16* _utf16_contents, Size _length,
  char* utf8_contents_);

Size utf8_to_utf16(const char* _utf8_contents, Size _length,
  Uint16* utf16_contents_);

RX_HINT_FORCE_INLINE Rx::String operator""_s(const char* _contents, Size _length) {
  return {_contents, _length};
}

} // namespace rx

#endif // RX_CORE_STRING_H
