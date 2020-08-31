#pragma once

#include "sanity.SanityEngine.g.h"

namespace winrt::sanity::implementation
{
    struct SanityEngine : SanityEngineT<SanityEngine>
    {
        SanityEngine() = default;

        void tick();
    };
}
namespace winrt::sanity::factory_implementation
{
    struct SanityEngine : SanityEngineT<SanityEngine, implementation::SanityEngine>
    {
    };
}
