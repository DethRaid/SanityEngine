#include "rx/core/types.h"
#include "rx/core/assert.h"
#include "rx/core/abort.h"

void* operator new(Size) {
  Rx::abort("operator new is disabled");
}

void* operator new[](Size) {
  Rx::abort("operator new[] is disabled");
}

void operator delete(void*) {
  Rx::abort("operator delete is disabled");
}

void operator delete(void*, Size) {
  Rx::abort("operator delete is disabled");
}

void operator delete[](void*) {
  Rx::abort("operator delete[] is disabled");
}

void operator delete[](void*, Size) {
  Rx::abort("operator delete[] is disabled");
}

extern "C" {
  void __cxa_pure_virtual() {
    Rx::abort("pure virtual function call");
  }

  static constexpr Uint8 k_complete{1 << 0};
  static constexpr Uint8 k_pending{1 << 1};

  bool __cxa_guard_acquire(Uint8* guard_) {
    if (guard_[1] == k_complete) {
      return false;
    }

    if (guard_[1] & k_pending) {
      Rx::abort("recursive initialization unsupported");
    }

    guard_[1] = k_pending;
    return true;
  }

  void __cxa_guard_release(Uint8* guard_) {
    guard_[1] = k_complete;
  }
}
