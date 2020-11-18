#pragma once

#include "core/Prelude.hpp"
#include "rx/core/concurrency/mutex.h"
#include "rx/core/utility/rex_forward.h"

template <typename T>
concept Mutex = requires(T a) {
    a.lock();
    a.unlock();
};

template <typename ResourceType, Mutex MutexType = Rx::Concurrency::Mutex>
class SynchronizedResourceAccessor {
    template <typename MutexResourceType, Mutex MutexType>
    friend class SynchronizedResource;

public:
    SynchronizedResourceAccessor(const SynchronizedResourceAccessor& other) = delete;
    SynchronizedResourceAccessor& operator=(const SynchronizedResourceAccessor& other) = delete;

    SynchronizedResourceAccessor(SynchronizedResourceAccessor&& old) noexcept = default;
    SynchronizedResourceAccessor& operator=(SynchronizedResourceAccessor&& old) noexcept = default;

    ResourceType* operator->();

    ResourceType& operator*();

    ~SynchronizedResourceAccessor();

private:
    SynchronizedResourceAccessor(MutexType& mutex_in, ResourceType& resource_in);

    MutexType* mutex;

    ResourceType* resource;
};

template <typename ResourceType, Mutex MutexType = Rx::Concurrency::Mutex>
class SynchronizedResource {
public:
    SynchronizedResource() {}

    explicit SynchronizedResource(MutexType&& mutex_in) : mutex{Rx::Utility::forward(mutex_in)} {}

    explicit SynchronizedResource(ResourceType&& resource_in) : resource{Rx::Utility::forward(resource_in)} {}

    SynchronizedResourceAccessor<ResourceType, MutexType> lock() { return {mutex, resource}; }

private:
    MutexType mutex;

    ResourceType resource;
};

template <typename ResourceType, Mutex MutexType>
ResourceType* SynchronizedResourceAccessor<ResourceType, MutexType>::operator->() {
    return resource;
}

template <typename ResourceType, Mutex MutexType>
ResourceType& SynchronizedResourceAccessor<ResourceType, MutexType>::operator*() {
    return *resource;
}

template <typename ResourceType, Mutex MutexType>
SynchronizedResourceAccessor<ResourceType, MutexType>::~SynchronizedResourceAccessor() {
    mutex->unlock();
}

template <typename ResourceType, Mutex MutexType>
SynchronizedResourceAccessor<ResourceType, MutexType>::SynchronizedResourceAccessor(MutexType& mutex_in, ResourceType& resource_in)
    : mutex{&mutex_in}, resource{&resource_in} {
    mutex->lock();
}
