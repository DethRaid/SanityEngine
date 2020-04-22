#pragma once

#include <mutex>

template <typename ResourceType>
class UnlockingResourceAccessor {
    template <typename MutexResourceType>
    friend class Mutex;

public:
    UnlockingResourceAccessor(const UnlockingResourceAccessor& other) = delete;
    UnlockingResourceAccessor& operator=(const UnlockingResourceAccessor& other) = delete;

    UnlockingResourceAccessor(UnlockingResourceAccessor&& old) noexcept = default;
    UnlockingResourceAccessor& operator=(UnlockingResourceAccessor&& old) noexcept = default;

    ResourceType* operator->();

    ~UnlockingResourceAccessor();

private:
    UnlockingResourceAccessor(std::mutex& mutex_in, ResourceType& resource_in);

    std::mutex* mutex;

    ResourceType* resource;
};

template <typename ResourceType>
class Mutex {
public:
    explicit Mutex(ResourceType&& resource_in);

    UnlockingResourceAccessor<ResourceType> lock();

private:
    std::mutex mutex;

    ResourceType resource;
};

template <typename ResourceType>
UnlockingResourceAccessor<ResourceType> Mutex<ResourceType>::lock() {
    mutex.lock();
    return {mutex, resource};
}

template <typename ResourceType>
ResourceType* UnlockingResourceAccessor<ResourceType>::operator->() {
    return resource;
}

template <typename ResourceType>
UnlockingResourceAccessor<ResourceType>::~UnlockingResourceAccessor() {
    mutex->unlock();
}

template <typename ResourceType>
UnlockingResourceAccessor<ResourceType>::UnlockingResourceAccessor(std::mutex& mutex_in, ResourceType& resource_in)
    : mutex{&mutex_in}, resource{&resource_in} {}

template <typename ResourceType>
Mutex<ResourceType>::Mutex(ResourceType&& resource_in) : resource{std::forward(resource_in)} {}
