#pragma once

#include <mutex>

template <typename T>
concept Mutex = requires(T a) {
    a.lock();
    a.unlock();
};

template <Mutex MutexType, typename ResourceType>
class UnlockingResourceAccessor {
    template <Mutex MutexType, typename MutexResourceType>
    friend class SynchronizedResource;

public:
    UnlockingResourceAccessor(const UnlockingResourceAccessor& other) = delete;
    UnlockingResourceAccessor& operator=(const UnlockingResourceAccessor& other) = delete;

    UnlockingResourceAccessor(UnlockingResourceAccessor&& old) noexcept = default;
    UnlockingResourceAccessor& operator=(UnlockingResourceAccessor&& old) noexcept = default;

    ResourceType* operator->();

    ~UnlockingResourceAccessor();

private:
    UnlockingResourceAccessor(MutexType& mutex_in, ResourceType& resource_in);

    MutexType* mutex;

    ResourceType* resource;
};

template <Mutex MutexType, typename ResourceType>
class SynchronizedResource {
public:
    explicit SynchronizedResource(MutexType&& mutex_in);

    explicit SynchronizedResource(ResourceType&& resource_in);

    UnlockingResourceAccessor<MutexType, ResourceType> lock();

private:
    MutexType mutex;

    ResourceType resource;
};

template <Mutex MutexType, typename ResourceType>
UnlockingResourceAccessor<MutexType, ResourceType> SynchronizedResource<MutexType, ResourceType>::lock() {
    mutex.lock();
    return {mutex, resource};
}

template <Mutex MutexType, typename ResourceType>
ResourceType* UnlockingResourceAccessor<MutexType, ResourceType>::operator->() {
    return resource;
}

template <Mutex MutexType, typename ResourceType>
UnlockingResourceAccessor<MutexType, ResourceType>::~UnlockingResourceAccessor() {
    mutex->unlock();
}

template <Mutex MutexType, typename ResourceType>
UnlockingResourceAccessor<MutexType, ResourceType>::UnlockingResourceAccessor(MutexType& mutex_in, ResourceType& resource_in)
    : mutex{&mutex_in}, resource{&resource_in} {}

template <Mutex MutexType, typename ResourceType>
SynchronizedResource<MutexType, ResourceType>::SynchronizedResource(MutexType&& mutex_in) : mutex{Rx::Utility::forward(mutex_in)} {}

template <Mutex MutexType, typename ResourceType>
SynchronizedResource<MutexType, ResourceType>::SynchronizedResource(ResourceType&& resource_in) : resource{std::forward(resource_in)} {}
