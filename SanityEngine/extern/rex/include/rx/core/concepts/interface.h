#ifndef RX_CORE_CONCEPTS_INTERFACE_H
#define RX_CORE_CONCEPTS_INTERFACE_H
#include "rx/core/concepts/no_copy.h" // no_copy
#include "rx/core/concepts/no_move.h" // no_move

namespace Rx::Concepts {

struct Interface
  : NoCopy
  , NoMove
{
  virtual ~Interface() = 0;
};

inline Interface::~Interface() {
  // { empty }
}

} // namespace rx::concepts

#endif // RX_CORE_CONCEPTS_INTERFACE_H
