#include "rx/core/memory/vma.h"
#include "rx/core/concurrency/scope_lock.h"

#if defined(RX_PLATFORM_POSIX)
#include <sys/mman.h> // mmap, munmap, mprotect, posix_madvise, MAP_{FAILED,HUGETLB}, PROT_{NONE,READ,WRITE}, POSIX_MADV_{WILLNEED,DONTNEED}
#elif defined(RX_PLATFORM_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h> // VirtualAlloc, VirtualFree, MEM_{RELEASE,COMMIT,UNCOMMIT}, PAGE_{READWRITE,READONLY}
#else
#error "missing VMA implementation"
#endif

namespace Rx::Memory {

void VMA::deallocate() {
  if (!is_valid()) {
    return;
  }

  const auto size = m_page_size * m_page_count;
#if defined(RX_PLATFORM_POSIX)
  RX_ASSERT(munmap(m_base, size) == 0, "munmap failed");
#elif defined(RX_PLATFORM_WINDOWS)
  RX_ASSERT(VirtualFree(m_base, 0, MEM_RELEASE), "VirtualFree failed");
#endif
}

bool VMA::allocate(Size _page_size, Size _page_count) {
  RX_ASSERT(!is_valid(), "already allocated");

#if defined(RX_PLATFORM_POSIX)
  int flags = MAP_PRIVATE | MAP_ANONYMOUS;

  // Determine the page size that closely matches |_page_size|.
  Size page_size = 4096;
  if (_page_size > 4096) {
    flags |= MAP_HUGETLB;
    if (_page_size > 2 * 1024 * 1024) {
      // 2 MiB pages.
      flags |= 21 << MAP_HUGE_SHIFT;
      page_size = 2 * 1024 * 1024;
    } else {
      // 1 GiB pages.
      flags |= 30 << MAP_HUGE_SHIFT;
      page_size = 1 * 1024 * 1024 * 1024;
    }
  }

  const auto size = page_size * _page_count;
  const auto map = mmap(nullptr, size, PROT_NONE, flags, -1, 0);
  if (map != MAP_FAILED) {
    // Ensure these pages are not comitted initially.
    if (posix_madvise(map, size, POSIX_MADV_DONTNEED) != 0) {
       munmap(map, size);
       return false;
     }

    m_page_size = page_size;
    m_page_count = _page_count;
    m_base = reinterpret_cast<Byte*>(map);
    return true;
  }
#elif defined(RX_PLATFORM_WINDOWS)
  // Using large pages on Windows requires the SecLockMemoryPrivilege privilege
  // which we cannot gurantee the user has. Just force 4 KiB pages.
  _page_size = 4096;

  const auto size = _page_size * _page_count;
  const auto map = VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS);
  if (map) {
    m_page_size = _page_size;
    m_page_count = _page_count;
    m_base = reinterpret_cast<Byte*>(map);
    return true;
  }
#endif
  return false;
}

bool VMA::commit(Range _range, bool _read, bool _write) {
  // Cannot commit memory unless one of |_read| or |_write| is true.
  if (!_read && !_write) {
    return false;

  }

  // Cannot commit memory unless in range of the VMA.
  if (!in_range(_range)) {
    return false;
  }

  const auto size = m_page_size * _range.count;
  const auto addr = m_base + m_page_size * _range.offset;

#if defined(RX_PLATFORM_POSIX)
  const auto prot = (_read ? PROT_READ : 0) | (_write ? PROT_WRITE : 0);
  // Ensure the mapping has the correct permissions.
  if (mprotect(addr, size, prot) == 0) {
    // Commit the memory.
    return posix_madvise(addr, size, POSIX_MADV_WILLNEED) == 0;
  }
#elif defined(RX_PLATFORM_WINDOWS)
  const DWORD protect = _write ? PAGE_READWRITE : PAGE_READONLY;
  // Commit the memory.
  return VirtualAlloc(addr, size, MEM_COMMIT, protect);
#endif
  return false;
}

bool VMA::uncommit(Range _range) {
  // Cannot commit memory unless in range of the VMA.
  if (!in_range(_range)) {
    return false;
  }

  const auto size = m_page_size * _range.count;
  const auto addr = m_base + m_page_size * _range.offset;
#if defined(RX_PLATFORM_POSIX)
  return posix_madvise(addr, size, POSIX_MADV_DONTNEED) == 0;
#elif defined(RX_PLATFORM_WINDOWS)
  return VirtualFree(addr, size, MEM_DECOMMIT);
#endif
  return false;
}

} // namespace rx::memory
