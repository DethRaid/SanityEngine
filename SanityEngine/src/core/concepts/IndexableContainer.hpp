#pragma once

#include <concepts>

template <typename ContainerType, typename IndexType, typename ValueType>
concept IndexableContainer = requires(ContainerType container, IndexType index) {
    { container[index] }
    ->std::convertible_to<ValueType>;
};
