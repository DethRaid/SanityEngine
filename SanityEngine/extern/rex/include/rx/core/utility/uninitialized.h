#ifndef RX_CORE_UTILITY_UNINITIALIZED_H
#define RX_CORE_UTILITY_UNINITIALIZED_H

namespace Rx::Utility {

// Special tag Type which represents a key-hole value for constructors and
// functions where you'd prefer to leave the contents uninitialized.
struct UninitializedTag { };

} // namespace rx::utility

#endif // RX_CORE_UTILITY_UNINITIALIZED_H
