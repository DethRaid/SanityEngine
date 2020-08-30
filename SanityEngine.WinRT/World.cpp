#include "pch.h"
#include "World.h"
#include "World.g.cpp"

namespace winrt::SanityEngine_WinRT::implementation
{
    int32_t World::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void World::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }
}
