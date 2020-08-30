#pragma once

#include "World.g.h"

namespace winrt::SanityEngine_WinRT::implementation
{
    struct World : WorldT<World>
    {
        World() = default;

        int32_t MyProperty();
        void MyProperty(int32_t value);
    };
}

namespace winrt::SanityEngine_WinRT::factory_implementation
{
    struct World : WorldT<World, implementation::World>
    {
    };
}
