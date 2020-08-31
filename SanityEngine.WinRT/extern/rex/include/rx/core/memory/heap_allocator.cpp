#include <stdlib.h> // malloc, realloc, free

#include "rx/core/memory/heap_allocator.h"

namespace Rx::Memory {

Global<HeapAllocator> HeapAllocator::s_instance{"system", "heap_allocator"};

Byte* HeapAllocator::allocate(Size _size) {
  return reinterpret_cast<Byte*>(malloc(_size));
}

Byte* HeapAllocator::reallocate(void* _data, Size _size) {
  return reinterpret_cast<Byte*>(realloc(_data, _size));
}

void HeapAllocator::deallocate(void* _data) {
  free(_data);
}

} // namespace rx::memory
