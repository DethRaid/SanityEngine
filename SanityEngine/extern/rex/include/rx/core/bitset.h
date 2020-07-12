#ifndef RX_CORE_BITSET_H
#define RX_CORE_BITSET_H
#include "rx/core/assert.h"

#include "rx/core/traits/is_same.h"
#include "rx/core/traits/return_type.h"

#include "rx/core/memory/system_allocator.h"

#include "rx/core/utility/exchange.h"

namespace Rx {

// 32-bit: 12 bytes
// 64-bit: 24 bytes
struct Bitset {
  using BitType = Uint64;

  static constexpr const BitType k_bit_one = 1;
  static constexpr const Size k_word_bits = 8 * sizeof(BitType);

  Bitset(Memory::Allocator& _allocator, Size _size);
  Bitset(Memory::Allocator& _allocator, const Bitset& _bitset);

  Bitset(Size _size);
  Bitset(Bitset&& bitset_);
  Bitset(const Bitset& _bitset);
  ~Bitset();

  Bitset& operator=(Bitset&& bitset_);
  Bitset& operator=(const Bitset& _bitset);

  // set |_bit|
  void set(Size _bit);

  // clear |_bit|
  void clear(Size _bit);

  // clear all bits
  void clear_all();

  // test if bit |_bit| is set
  bool test(Size _bit) const;

  // the amount of bits
  Size size() const;

  // count the # of set bits
  Size count_set_bits() const;

  // count the # of unset bits
  Size count_unset_bits() const;

  // find the index of the first set bit
  Size find_first_set() const;

  // find the index of the first unset bit
  Size find_first_unset() const;

  // iterate bitset invoking |_function| with index of each set bit
  template<typename F>
  void each_set(F&& _function) const;

  // iterate bitset invoking |_function| with index of each unset bit
  template<typename F>
  void each_unset(F&& _function) const;

  constexpr Memory::Allocator& allocator() const;

private:
  static Size bytes_for_size(Size _size);

  static Size index(Size bit);
  static Size offset(Size bit);

  Memory::Allocator* m_allocator;
  Size m_size;
  BitType* m_data;
};

inline Bitset::Bitset(Size _size)
  : Bitset{Memory::SystemAllocator::instance(), _size}
{
}

inline Bitset::Bitset(const Bitset& _bitset)
  : Bitset{_bitset.allocator(), _bitset}
{
}

inline Bitset::Bitset(Bitset&& bitset_)
  : m_allocator{bitset_.m_allocator}
  , m_size{Utility::exchange(bitset_.m_size, 0)}
  , m_data{Utility::exchange(bitset_.m_data, nullptr)}
{
}

inline Bitset::~Bitset() {
  m_allocator->deallocate(m_data);
}

inline void Bitset::set(Size _bit) {
  RX_ASSERT(_bit < m_size, "out of bounds");
  m_data[index(_bit)] |= k_bit_one << offset(_bit);
}

inline void Bitset::clear(Size _bit) {
  RX_ASSERT(_bit < m_size, "out of bounds");
  m_data[index(_bit)] &= ~(k_bit_one << offset(_bit));
}

inline bool Bitset::test(Size _bit) const {
  RX_ASSERT(_bit < m_size, "out of bounds");
  return !!(m_data[index(_bit)] & (k_bit_one << offset(_bit)));
}

inline Size Bitset::size() const {
  return m_size;
}

inline Size Bitset::bytes_for_size(Size _size) {
  return sizeof(BitType) * (_size / k_word_bits + 1);
}

inline Size Bitset::index(Size _bit) {
  return _bit / k_word_bits;
}

inline Size Bitset::offset(Size _bit) {
  return _bit % k_word_bits;
}

template<typename F>
inline void Bitset::each_set(F&& _function) const {
  for (Size i{0}; i < m_size; i++) {
    if (test(i)) {
      if constexpr (traits::is_same<bool, traits::return_type<F>>) {
        if (!_function(i)) {
          return;
        }
      } else {
        _function(i);
      }
    }
  }
}

template<typename F>
inline void Bitset::each_unset(F&& _function) const {
  for (Size i{0}; i < m_size; i++) {
    if (!test(i)) {
      if constexpr (traits::is_same<bool, traits::return_type<F>>) {
        if (!_function(i)) {
          return;
        }
      } else {
        _function(i);
      }
    }
  }
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Bitset::allocator() const {
  return *m_allocator;
}

} // namespace rx

#endif // RX_CORE_BITSET_H
