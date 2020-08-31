#include <string.h> // strstr, strlen, memcpy

#include "rx/core/string_table.h"
#include "rx/core/string.h"

#include "rx/core/hints/unlikely.h"

namespace Rx {

StringTable::StringTable(Memory::Allocator& _allocator, const char* _data, Size _size)
  : m_data{_allocator, _size}
{
  RX_ASSERT(_data[_size] == '\0', "missing null-terminator");
  memcpy(m_data.data(), _data, _size);
}

Optional<Size> StringTable::find(const char* _string) const {
  if (RX_HINT_UNLIKELY(m_data.is_empty())) {
    return nullopt;
  }

  if (const char* search = strstr(m_data.data(), _string)) {
    return static_cast<Size>(search - m_data.data());
  }
  return nullopt;
}

Optional<Size> StringTable::add(const char* _string, Size _size) {
  const Size index = m_data.size();
  const Size total = _size + 1;
  if (m_data.resize(index + total, Utility::UninitializedTag{})) {
    memcpy(m_data.data() + index, _string, total);
    return index;
  }
  return nullopt;
}

Optional<Size> StringTable::insert(const char* _string, Size _size) {
  if (auto search = find(_string)) {
    return *search;
  }
  return add(_string, _size);
}

Optional<Size> StringTable::insert(const char* _string) {
  return insert(_string, strlen(_string));
}

Optional<Size> StringTable::insert(const String& _string) {
  return insert(_string.data(), _string.size());
}

} // namespace rx
