#include <stdlib.h> // malloc, realloc, free
#include <string.h> // memmove
#include <stddef.h> // max_align_t.

#include "rx/core/memory/heap_allocator.h"

#include "rx/core/hints/unlikely.h"
#include "rx/core/hints/likely.h"
#include "rx/core/hints/unreachable.h"

namespace Rx::Memory {

Global<HeapAllocator> HeapAllocator::s_instance{"system", "heap_allocator"};

// The standard heap allocator in C and C++ is defined to return memory that is
// suitably aligned for alignof(max_align_t).
//
// Unfortunately we need all allocations to be aligned by |Allocator::ALIGNMENT|
// which isn't necessarily the same. Similarly, some platforms like Emscripten
// violate the standard where alignof(max_align_t) is larger than the alignment
// given by it's allocator.
//
// We don't want to pay for additional overhead unless the implementation
// requires we over-allocate and align stuff to |Allocator::Alignment|.
//
// Check if we need to perform manual alignment of allocations.
#if defined(RX_PLATFORM_EMSCRIPTEN)
static inline constexpr const bool NEEDS_MANUAL_ALIGNMENT = true;
#else
static inline constexpr const bool NEEDS_MANUAL_ALIGNMENT =
  alignof(max_align_t) < Allocator::ALIGNMENT || alignof(max_align_t) % Allocator::ALIGNMENT != 0;
#endif

// When manual alignment is needed we store the size of the allocation rounded
// to a multiple of |Allocator::ALIGNMENT|. The remaining bits are used to store
// the difference between the aligned pointer and the base pointer. This gives
// a "header" of type |Size|.
#define HEADER(_data) \
  (reinterpret_cast<Size*>(_data)[-1])
#define SIZE(_data) \
  (HEADER(_data) & ~(ALIGNMENT - 1))
#define BASE(_data) \
  (reinterpret_cast<Byte*>(_data) - sizeof(Size) - (HEADER(_data) & (ALIGNMENT - 1)))

Byte* HeapAllocator::allocate(Size _size) {
  if constexpr (NEEDS_MANUAL_ALIGNMENT) {
    const auto size = round_to_alignment(_size);
    const auto base = reinterpret_cast<Byte*>(malloc(size + sizeof(Size) + ALIGNMENT));
    if (RX_HINT_UNLIKELY(!base)) {
      return nullptr;
    }
    auto aligned = round_to_alignment(base + sizeof(Size));
    HEADER(aligned) = size | (aligned - base - sizeof(Size));
    return aligned;
  } else {
    return reinterpret_cast<Byte*>(malloc(_size));
  }

  RX_HINT_UNREACHABLE();
}

Byte* HeapAllocator::reallocate(void* _data, Size _size) {
  if constexpr (NEEDS_MANUAL_ALIGNMENT) {
    if (RX_HINT_UNLIKELY(!_data)) {
      return allocate(_size);
    }

    const auto size = round_to_alignment(_size);
    const auto original = BASE(_data);
    const auto original_size = SIZE(_data);
    const auto shift = reinterpret_cast<Byte*>(_data) - original;
    const auto resize = reinterpret_cast<Byte*>(realloc(original, size + sizeof(Size) + ALIGNMENT));
    if (RX_HINT_UNLIKELY(!resize)) {
      return nullptr;
    }
    const auto aligned = round_to_alignment(resize + sizeof(Size));

    // There's a relative shift that could occur under reallocations while
    // trying to maintain alignment. This shift requires moving the contents
    // in-place. This will only move what is necessary to account for the
    // relative shift of the contents.
    if (shift != aligned - resize) {
      memmove(aligned, resize + shift, original_size);
    }

    HEADER(aligned) = size | (aligned - resize - sizeof(Size));

    return aligned;
  } else {
    return reinterpret_cast<Byte*>(realloc(_data, _size));
  }

  RX_HINT_UNREACHABLE();
}

void HeapAllocator::deallocate(void* _data) {
  if constexpr (NEEDS_MANUAL_ALIGNMENT) {
    if (RX_HINT_LIKELY(_data)) {
      free(BASE(_data));
    }
  } else {
    free(_data);
  }
}

} // namespace rx::memory
