#include "rx/core/memory/allocator.h"

namespace Rx::Memory {

Byte* Allocator::allocate(Size _size, Size _count) {
  // Would |_size * _count| overflow?
  if (_size && _count > -1_z / _size) {
    return nullptr;
  }

  return allocate(_size * _count);
}

} // namespace Rx::Memory
