#ifndef RX_CORE_CONCEPTS_NO_MOVE_H
#define RX_CORE_CONCEPTS_NO_MOVE_H

namespace Rx::Concepts {

struct NoMove {
  NoMove() = default;
  ~NoMove() = default;
  NoMove(NoMove&&) = delete;
  void operator=(NoMove&&) = delete;
};

} // namespace rx::concepts

#endif // RX_CORE_CONCEPTS_NO_MOVE_H
