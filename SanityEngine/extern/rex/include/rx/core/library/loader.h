#ifndef RX_CORE_LIBRARY_LOADER_H
#define RX_CORE_LIBRARY_LOADER_H
#include "rx/core/string.h"
#include "rx/core/optional.h"

namespace Rx::library {

struct Loader {
  RX_MARK_NO_COPY(Loader);

  Loader(const String& _file_name);
  Loader(Memory::Allocator& _allocator, const String& _file_name);
  Loader(Loader&& _loader);
  ~Loader();

  Loader& operator=(Loader&& loader_);

  operator bool() const;
  bool is_valid() const;

  // Link the function pointer given by |function_| up with the symbol in
  // the library given by |_symbol_name|. Returns true on success, false on
  // failure.
  template<typename F>
  bool link(F*& function_, const char* _symbol_name) const;

  template<typename F>
  bool link(F*& function_, const String& _symbol_name) const;

private:
  void close_unlocked();

  // Returns nullptr when |_symbol_name| isn't found.
  void* address_of(const char* _symbol_name) const;

  Memory::Allocator* m_allocator;
  void* m_handle;
};

inline Loader::Loader(const String& _file_name)
  : Loader{Memory::SystemAllocator::instance(), _file_name}
{
}

inline Loader::Loader(Loader&& loader_)
  : m_allocator{loader_.m_allocator}
  , m_handle{Utility::exchange(loader_.m_handle, nullptr)}
{
}

inline Loader::operator bool() const {
  return m_handle != nullptr;
}

inline bool Loader::is_valid() const {
  return m_handle != nullptr;
}

template<typename F>
inline bool Loader::link(F*& function_, const char* _symbol_name) const {
  RX_ASSERT(m_handle, "no handle");
  if (const auto proc = address_of(_symbol_name)) {
    *reinterpret_cast<void**>(&function_) = proc;
    return true;
  }
  return false;
}

template<typename F>
inline bool Loader::link(F*& function_, const String& _symbol_name) const {
  return link(function_, _symbol_name.data());
}

} // namespace rx::library

#endif // RX_CORE_LIBRARY_LOADER_H
