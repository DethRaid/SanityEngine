#pragma once

#include "core/types.hpp"
#include "rx/core/vector.h"

template <typename ValueType>
class VectorHandle {
public:
    VectorHandle(Rx::Vector<ValueType>* container_in, Size index_in);

    ValueType* operator->();

    const ValueType* operator->() const;

    [[nodiscard]] Size get_index() const;

private:
    Rx::Vector<ValueType>* vector{nullptr};

    Size index{};
};

template <typename ValueType>
VectorHandle<ValueType>::VectorHandle(Rx::Vector<ValueType>* container_in, const Size index_in)
    : vector{container_in}, index{index_in} {}

template <typename ValueType>
ValueType* VectorHandle<ValueType>::operator->() {
    return &(*vector)[index];
}

template <typename ValueType>
const ValueType* VectorHandle<ValueType>::operator->() const {
    return &(*vector)[index];
}

template <typename ValueType>
Size VectorHandle<ValueType>::get_index() const {
    return index;
}
