#ifndef RX_CORE_HASH_H
#define RX_CORE_HASH_H
#include "rx/core/types.h"

#include "rx/core/traits/detect.h"
#include "rx/core/traits/underlying_type.h"

#include "rx/core/utility/declval.h"

#include "rx/core/hints/unreachable.h"

namespace Rx {

template<typename T>
struct Hash {
  template<typename U>
  using HasHash = decltype(Utility::declval<U>().hash());

  Size operator()(const T& _value) const {
    if constexpr (traits::detect<T, HasHash>) {
      // The Type has a hash member function we can use.
      return _value.hash();
    } else if constexpr (traits::is_enum<T>) {
      // We can convert the enum to the underlying Type and forward to existing
      // integer hash specialization.
      using underlying_type = traits::underlying_type<T>;
      return Hash<underlying_type>{}(static_cast<underlying_type>(_value));
    } else {
      static_assert("implement size_t T::hash()");
    }
    RX_HINT_UNREACHABLE();
  }
};

inline constexpr Size hash_bool(bool _value) {
  return _value ? 1231 : 1237;
}

inline constexpr Size hash_uint8(Uint8 _value) {
  Size hash = _value * 251;
  hash += ~(_value << 3);
  hash ^= (_value >> 1);
  hash += ~(_value << 7);
  hash ^= (_value >> 6);
  hash += (_value << 2);
  return hash;
}

inline constexpr Size hash_uint16(Uint16 _value) {
  const Size z = (_value << 8) | (_value >> 8);
  Size hash = z;
  hash += ~(z << 5);
  hash ^= (z >> 2);
  hash += ~(z << 13);
  hash ^= (z >> 10);
  hash += ~(z << 4);
  hash = (hash << 10) | (hash >> 10);
  return hash;
}

inline constexpr Size hash_uint32(Uint32 _value) {
  _value = (_value ^ 61) ^ (_value >> 16);
  _value = _value + (_value << 3);
  _value = _value ^ (_value >> 4);
  _value = _value * 0x27D4EB2D;
  _value = _value ^ (_value >> 15);
  return static_cast<Size>(_value);
}

inline constexpr Size hash_uint64(Uint64 _value) {
  _value = (~_value) + (_value << 21);
  _value = _value ^ (_value >> 24);
  _value = (_value + (_value << 3)) + (_value << 8);
  _value = _value ^ (_value >> 14);
  _value = (_value + (_value << 2)) + (_value << 4);
  _value = _value ^ (_value << 28);
  _value = _value + (_value << 31);
  return static_cast<Size>(_value);
}

template<>
struct Hash<bool> {
  constexpr Size operator()(bool _value) const {
    return hash_bool(_value);
  }
};

template<>
struct Hash<signed char> {
  constexpr Size operator()(signed char _value) const {
    return hash_uint8(static_cast<Uint8>(_value));
  }
};

template<>
struct Hash<signed short> {
  constexpr Size operator()(signed short _value) const {
    return hash_uint16(static_cast<Uint16>(_value));
  }
};

template<>
struct Hash<signed int> {
  constexpr Size operator()(unsigned int _value) const {
    return hash_uint32(static_cast<Uint32>(_value));
  }
};

template<>
struct Hash<signed long> {
  constexpr Size operator()(signed long _value) const {
    if constexpr (sizeof _value == 8) {
      return hash_uint64(static_cast<Uint64>(_value));
    } else {
      return hash_uint32(static_cast<Uint32>(_value));
    }
  }
};

template<>
struct Hash<signed long long> {
  constexpr Size operator()(signed long long _value) const {
    return hash_uint64(static_cast<Uint64>(_value));
  }
};

template<>
struct Hash<unsigned char> {
  constexpr Size operator()(unsigned char _value) const {
    return hash_uint8(_value);
  }
};

template<>
struct Hash<unsigned short> {
  constexpr Size operator()(unsigned short _value) const {
    return hash_uint16(_value);
  }
};

template<>
struct Hash<unsigned int> {
  constexpr Size operator()(unsigned int _value) const {
    return hash_uint32(_value);
  }
};

template<>
struct Hash<unsigned long> {
  constexpr Size operator()(unsigned long _value) const {
    if constexpr (sizeof _value == 8) {
      return hash_uint64(static_cast<Uint64>(_value));
    } else {
      return hash_uint32(static_cast<Uint32>(_value));
    }
  }
};

template<>
struct Hash<unsigned long long> {
  constexpr Size operator()(unsigned long long _value) const {
    return hash_uint64(static_cast<Uint64>(_value));
  }
};

template<>
struct Hash<Float32> {
  Size operator()(Float32 _value) const {
    const union { Float32 f; Uint32 u; } cast{_value};
    return Hash<Uint32>{}(cast.u);
  };
};

template<>
struct Hash<Float64> {
  Size operator()(Float64 _value) const {
    const union { Float64 f; Uint64 u; } cast{_value};
    return Hash<Uint64>{}(cast.u);
  };
};

template<typename T>
struct Hash<T*> {
  constexpr Size operator()(T* _value) const {
    if constexpr (sizeof _value == 8) {
      return hash_uint64(reinterpret_cast<Uint64>(_value));
    } else {
      return hash_uint32(reinterpret_cast<Uint32>(_value));
    }
  }
};

inline constexpr Size hash_combine(Size _hash1, Size _hash2) {
  return _hash1 ^ (_hash2 + 0x9E3779B9 + (_hash1 << 6) + (_hash1 >> 2));
}

} // namespace rx

#endif // RX_CORE_HASH_H
