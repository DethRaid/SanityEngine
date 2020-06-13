#ifndef RX_CORE_MEMORY_VMA_H
#define RX_CORE_MEMORY_VMA_H
#include "rx/core/types.h"
#include "rx/core/assert.h"

#include "rx/core/utility/exchange.h"

#include "rx/core/concepts/no_copy.h"

namespace Rx::Memory {

struct VMA
  : Concepts::NoCopy
{
  constexpr VMA();
  constexpr VMA(Byte* _base, Size _page_size, Size _page_count);
  VMA(VMA&& vma_);

  ~VMA();
  VMA& operator=(VMA&& vma_);

  struct Range {
    Size offset;
    Size count;
  };

  [[nodiscard]] bool allocate(Size _page_size, Size _page_count);
  [[nodiscard]] bool commit(Range _range, bool _read, bool _write);
  [[nodiscard]] bool uncommit(Range _range);

  Byte* base() const;
  Byte* page(Size _index) const;

  Size page_count() const;
  Size page_size() const;

  bool is_valid() const;
  bool in_range(Range _range) const;

  Byte* release();

private:
  void deallocate();

  Byte* m_base;

  Size m_page_size;
  Size m_page_count;
};

inline constexpr VMA::VMA()
  : VMA{nullptr, 0, 0}
{
}

inline constexpr VMA::VMA(Byte* _base, Size _page_size, Size _page_count)
  : m_base{_base}
  , m_page_size{_page_size}
  , m_page_count{_page_count}
{
}

inline VMA::VMA(VMA&& vma_)
  : m_base{Utility::exchange(vma_.m_base, nullptr)}
  , m_page_size{Utility::exchange(vma_.m_page_size, 0)}
  , m_page_count{Utility::exchange(vma_.m_page_count, 0)}
{
}

inline VMA::~VMA() {
  deallocate();
}

inline VMA& VMA::operator=(VMA&& vma_) {
  deallocate();

  m_base = Utility::exchange(vma_.m_base, nullptr);
  m_page_size = Utility::exchange(vma_.m_page_size, 0);
  m_page_count = Utility::exchange(vma_.m_page_count, 0);

  return *this;
}

inline Byte* VMA::base() const {
  RX_ASSERT(is_valid(), "unallocated");
  return m_base;
}

inline Byte* VMA::page(Size _page) const {
  return base() + m_page_size * _page;
}

inline Size VMA::page_count() const {
  return m_page_count;
}

inline Size VMA::page_size() const {
  return m_page_size;
}

inline bool VMA::is_valid() const {
  return m_base != nullptr;
}

inline bool VMA::in_range(Range _range) const {
  return _range.offset + _range.count <= m_page_count;
}

inline Byte* VMA::release() {
  m_page_size = 0;
  m_page_count = 0;
  return Utility::exchange(m_base, nullptr);
}

} // namespace rx::memory

#endif // RX_CORE_MEMORY_VMA_H
