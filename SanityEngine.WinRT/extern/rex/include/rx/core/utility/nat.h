#ifndef RX_CORE_UTILITY_NAT_H
#define RX_CORE_UTILITY_NAT_H

namespace Rx::Utility {

// Special tag Type which represents a NAT "not-a-Type". Commonly used as a
// stand in Type in a union which constains non-trivial to construct members to
// enable constexpr initialization.
//
// One such example is Optional<T> which has constexpr constructors because the
// uninitialized case initializes an embedded NAT.
struct Nat { };

} // namespace rx::utility

#endif // RX_CORE_UTILITY_NAT_H
