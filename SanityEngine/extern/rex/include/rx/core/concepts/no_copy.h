#ifndef RX_CORE_CONCEPTS_NO_COPY_H
#define RX_CORE_CONCEPTS_NO_COPY_H

namespace Rx::Concepts {

struct NoCopy {
  NoCopy() = default;
  ~NoCopy() = default;
  NoCopy(const NoCopy&) = delete;
  void operator=(const NoCopy&) = delete;
};

} // namespace rx::concepts

#endif // RX_CORE_CONCEPTS_NO_COPY_H
