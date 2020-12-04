#pragma once

#include "core/types.hpp"
#include "rx/core/vector.h"

template <typename ValueType>
class VectorHandle {
public:
    VectorHandle(Rx::Vector<ValueType>* container_in, Size index_in);

    ValueType* operator->();

    [[nodiscard]] Size get_index() const;

private:
    Rx::Vector<ValueType>* container{nullptr};

    Size index{};
};

template <typename ValueType>
VectorHandle<ValueType>::VectorHandle(Rx::Vector<ValueType>* container_in, const Size index_in)
    : container{container_in}, index{index_in} {}

template <typename ValueType>
ValueType* VectorHandle<ValueType>::operator->() {
    return &(*container)[index];
}

template <typename ValueType>
Size VectorHandle<ValueType>::get_index() const {
    return index;
}
