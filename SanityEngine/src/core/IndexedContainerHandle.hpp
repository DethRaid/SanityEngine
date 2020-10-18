#pragma once

#include "core/concepts/IndexableContainer.hpp"
#include "rx/core/vector.h"
#include "rx/core/map.h"

template <IndexableContainer ContainerType, typename IndexType, typename ValueType>
class IndexedContainerHandle {
public:
    IndexedContainerHandle(ContainerType* container_in, IndexType index_in);

    ValueType* operator->();

private:
    ContainerType* container{nullptr};

    IndexType index{};
};

template<typename IndexType, typename ValueType>
using MapHandle = IndexedContainerHandle<Rx::Map<IndexType, ValueType>, IndexType, ValueType>;

template<typename ValueType>
using VectorHandle = IndexedContainerHandle<Rx::Vector<ValueType>, Rx::Size, ValueType>;

template <IndexableContainer ContainerType, typename IndexType, typename ValueType>
IndexedContainerHandle<ContainerType, IndexType, ValueType>::IndexedContainerHandle(ContainerType* container_in, IndexType index_in)
    : container{container_in}, index{index_in} {}

template <IndexableContainer ContainerType, typename IndexType, typename ValueType>
ValueType* IndexedContainerHandle<ContainerType, IndexType, ValueType>::operator->() {
    return &(*container)[index];
}
