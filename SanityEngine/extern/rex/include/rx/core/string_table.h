#ifndef RX_CORE_STRING_TABLE_H
#define RX_CORE_STRING_TABLE_H
#include "rx/core/vector.h"
#include "rx/core/optional.h"

namespace Rx {

struct String;

struct StringTable {
  constexpr StringTable();
  constexpr StringTable(Memory::Allocator& _allocator);

  // Construct a string table from raw string data.
  StringTable(Vector<char>&& data_);
  StringTable(Memory::Allocator& _allocator, const char* _data, Size _size);

  StringTable(StringTable&& string_table_);
  StringTable(const StringTable& _string_table);

  StringTable& operator=(StringTable&& string_table_);
  StringTable& operator=(const StringTable& _string_table);

  Optional<Size> insert(const char* _string, Size _length);
  Optional<Size> insert(const char* _string);
  Optional<Size> insert(const String& _string);

  const char* operator[](Size _index) const;

  const char* data() const;
  Size size() const;

  void clear();

  constexpr Memory::Allocator& allocator() const;

private:
  Optional<Size> find(const char* _string) const;
  Optional<Size> add(const char* _string, Size _size);

  Vector<char> m_data;
};

inline constexpr StringTable::StringTable()
  : StringTable{Memory::SystemAllocator::instance()}
{
}

inline constexpr StringTable::StringTable(Memory::Allocator& _allocator)
  : m_data{_allocator}
{
}

inline StringTable::StringTable(Vector<char>&& data_)
  : m_data{Utility::move(data_)}
{
}

inline StringTable::StringTable(StringTable&& string_table_)
  : m_data{Utility::move(string_table_.m_data)}
{
}

inline StringTable::StringTable(const StringTable& _string_table)
  : m_data{_string_table.m_data}
{
}

inline StringTable& StringTable::operator=(StringTable&& string_table_) {
  m_data = Utility::move(string_table_.m_data);
  return *this;
}

inline StringTable& StringTable::operator=(const StringTable& _string_table) {
  m_data = _string_table.m_data;
  return *this;
}

inline const char* StringTable::operator[](Size _index) const {
  return &m_data[_index];
}

inline const char* StringTable::data() const {
  return m_data.data();
}

inline Size StringTable::size() const {
  return m_data.size();
}

inline void StringTable::clear() {
  m_data.clear();
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& StringTable::allocator() const {
  return m_data.allocator();
}

} // namespace rx

#endif // RX_CORE_STRING_TABLE_H
