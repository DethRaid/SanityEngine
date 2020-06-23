#pragma once

#include <rx/core/concurrency/mutex.h>

template <typename T>
concept Mutex = requires(T a) {
    a.lock();
    a.unlock();
};

template <typename ResourceType, Mutex MutexType = Rx::Concurrency::Mutex>
class UnlockingResourceAccessor {
    template <typename MutexResourceType, Mutex MutexType>
    friend class SynchronizedResource;

public:
    UnlockingResourceAccessor(const UnlockingResourceAccessor& other) = delete;
    UnlockingResourceAccessor& operator=(const UnlockingResourceAccessor& other) = delete;

    UnlockingResourceAccessor(UnlockingResourceAccessor&& old) noexcept = default;
    UnlockingResourceAccessor& operator=(UnlockingResourceAccessor&& old) noexcept = default;

    ResourceType* operator->();

    ResourceType& operator*();

    ~UnlockingResourceAccessor();

private:
    UnlockingResourceAccessor(MutexType& mutex_in, ResourceType& resource_in);

    MutexType* mutex;

    ResourceType* resource;
};

template <typename ResourceType, Mutex MutexType = Rx::Concurrency::Mutex>
class SynchronizedResource {
public:
    SynchronizedResource() {}

    explicit SynchronizedResource(MutexType&& mutex_in) : mutex{Rx::Utility::forward(mutex_in)} {}

    explicit SynchronizedResource(ResourceType&& resource_in) : resource{Rx::Utility::forward(resource_in)} {}

    UnlockingResourceAccessor<ResourceType, MutexType> lock() { return {mutex, resource}; }

private:
    MutexType mutex;

    ResourceType resource;
};

template <typename ResourceType, Mutex MutexType>
ResourceType* UnlockingResourceAccessor<ResourceType, MutexType>::operator->() {
    return resource;
}

template <typename ResourceType, Mutex MutexType>
ResourceType& UnlockingResourceAccessor<ResourceType, MutexType>::operator*() {
    return *resource;
}

template <typename ResourceType, Mutex MutexType>
UnlockingResourceAccessor<ResourceType, MutexType>::~UnlockingResourceAccessor() {
    mutex->unlock();
}

template <typename ResourceType, Mutex MutexType>
UnlockingResourceAccessor<ResourceType, MutexType>::UnlockingResourceAccessor(MutexType& mutex_in, ResourceType& resource_in)
    : mutex{&mutex_in}, resource{&resource_in} {
    mutex->lock();
}
