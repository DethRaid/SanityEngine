#ifndef RX_CORE_SERIALIZE_HEADER_H
#define RX_CORE_SERIALIZE_HEADER_H
#include "rx/core/types.h"

namespace Rx::serialize {

struct Header {
  constexpr Header();

  // The magic string, always "REX".
  Byte magic[4];

  // The serialization version number.
  Uint32 version;

  // The size of the data and string tables, respectively.
  Uint64 data_size;
  Uint64 string_size;
};

inline constexpr Header::Header()
  : magic{'R', 'E', 'X', '\0'}
  , version{0}
  , data_size{0}
  , string_size{0}
{
}

} // namespace rx::serialize

#endif // RX_CORE_SERIALIZE_HEADER_H
