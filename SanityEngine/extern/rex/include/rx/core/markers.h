#ifndef RX_CORE_MARKERS__H
#define RX_CORE_MARKERS__H

// Delete copy assignment operator for |T|
#define RX_MARK_NO_COPY_ASSIGN(T) \
  constexpr T& operator=(const T&) = delete; \
  constexpr T& operator=(const T&) const = delete; \
  constexpr T& operator=(const T&) volatile = delete; \
  constexpr T& operator=(const T&) const volatile = delete

// Delete move assignment operator for |T|
#define RX_MARK_NO_MOVE_ASSIGN(T) \
  constexpr T& operator=(T&&) = delete; \
  constexpr T& operator=(T&&) const = delete; \
  constexpr T& operator=(T&&) volatile = delete; \
  constexpr T& operator=(T&&) const volatile = delete

// Delete copy constructor for |T|
#define RX_MARK_NO_COPY_CONSTRUCT(T) \
  constexpr T(const T&) = delete

// Delete move constructor for |T|
#define RX_MARK_NO_MOVE_CONSTRUCT(T) \
  constexpr T(T&&) = delete

// Disable copy assign and construct for |T|
#define RX_MARK_NO_COPY(T) \
  RX_MARK_NO_COPY_ASSIGN(T); \
  RX_MARK_NO_COPY_CONSTRUCT(T)

// Disable move assign and construct for |T|
#define RX_MARK_NO_MOVE(T) \
  RX_MARK_NO_MOVE_ASSIGN(T); \
  RX_MARK_NO_MOVE_CONSTRUCT(T)

// Mark |T| an interface.
#define RX_MARK_INTERFACE(T) \
  RX_MARK_NO_COPY(T); \
  RX_MARK_NO_MOVE(T); \
  constexpr T() = default; \
  virtual ~T() = default

#endif // RX_CORE_MARKERS_H
