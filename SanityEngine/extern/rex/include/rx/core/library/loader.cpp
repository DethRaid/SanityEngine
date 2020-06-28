#include "rx/core/library/loader.h"
#include "rx/core/hints/likely.h"
#include "rx/core/concurrency/scope_lock.h"

#if defined(RX_PLATFORM_POSIX)
#include <dlfcn.h> // dlopen, dlclose, dlsym
#elif defined(RX_PLATFORM_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h> // LoadLibraryW, FreeLibrary, GetProcAddress
#endif

namespace Rx::library {

// The dynamic-linker is not thread-safe on most systems. To avoid potential
// issues we maintain a single global lock on the dynamic-linker interfaces
// here so that only one thread can use it at a time.
//
// This will of course only work if every one agrees to use this interface.
static Concurrency::SpinLock g_lock;

Loader::Loader(Memory::Allocator& _allocator, const String& _file_name)
  : m_allocator{_allocator}
  , m_handle{nullptr}
{
  // Discourage passing file extensions on the filename.
  RX_ASSERT(!_file_name.ends_with(".dll") && !_file_name.ends_with(".so"),
    "library filename should not contain file extension");

  Concurrency::ScopeLock lock{g_lock};
#if defined(RX_PLATFORM_POSIX)
  const auto path = String::format(m_allocator, "%s.so", _file_name);
  const auto name = path.data();
  if (auto handle = dlopen(name, RTLD_NOW | RTLD_LOCAL)) {
    m_handle = handle;
  } else if (!_file_name.begins_with("lib")) {
    // There's a non-enforced convention of using a "lib" prefix for naming
    // libraries, attempt this when the above fails and the library name
    // doesn't begin with such a prefix.
    const auto path = String::format(m_allocator, "lib%s.so", _file_name);
    const auto name = path.data();
    m_handle = dlopen(name, RTLD_NOW | RTLD_LOCAL);
  }
#elif defined(RX_PLATFORM_WINDOWS)
  const auto path_utf8 = String::format(_allocator, "%s.dll", _file_name);
  const auto path_utf16 = path_utf8.to_utf16();
  const auto name = reinterpret_cast<LPCWSTR>(path_utf16.data());

  m_handle = static_cast<void*>(LoadLibraryW(name));
#endif
}

Loader::~Loader() {
  Concurrency::ScopeLock lock{g_lock};
  close_unlocked();
}

Loader& Loader::operator=(Loader&& loader_) {
  Concurrency::ScopeLock lock{g_lock};
  close_unlocked();
  m_allocator = loader_.m_allocator;
  m_handle = Utility::exchange(loader_.m_handle, nullptr);
  return *this;
}

void* Loader::address_of(const char* _symbol_name) const {
  Concurrency::ScopeLock lock{g_lock};
  if (RX_HINT_LIKELY(m_handle)) {
#if defined(RX_PLATFORM_POSIX)
    if (auto function = dlsym(m_handle, _symbol_name)) {
      return function;
    } else {
      // POSIX systems export symbols in accordance to what the toolchain
      // defines for __USER_LABEL_PREFIX__. This is almost always an empty
      // macro, however some define it as a single underscore.
      //
      // Search again with the underscore.
      const auto symbol = String::format(m_allocator, "_%s", _symbol_name);
      if (auto function = dlsym(m_handle, symbol.data())) {
        return reinterpret_cast<void*>(function);
      }
    }
#elif defined(RX_PLATFORM_WINDOWS)
    auto handle = static_cast<HMODULE>(m_handle);
    if (auto function = GetProcAddress(handle, _symbol_name)) {
      return reinterpret_cast<void*>(function);
    }
#endif
  }
  return nullptr;
}

void Loader::close_unlocked() {
  if (RX_HINT_LIKELY(m_handle)) {
#if defined(RX_PLATFORM_POSIX)
    dlclose(m_handle);
#elif defined(RX_PLATFORM_WINDOWS)
    FreeLibrary(static_cast<HMODULE>(m_handle));
#endif
  }
}

} // namespace rx::library
