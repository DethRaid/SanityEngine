#pragma once

#include "sanity.SanityEngine.g.h"

namespace winrt::sanity::implementation {
    using Windows::UI::Xaml::Controls::ISwapChainPanel;

    struct SanityEngine : SanityEngineT<SanityEngine> {
        SanityEngine();

        void SetRenderSurface(Windows::UI::Xaml::Controls::SwapChainPanel const& renderSurface);

        void Tick(double deltaTime);

    private:
        void create_engine();
    };
} // namespace winrt::sanity::implementation

namespace winrt::sanity::factory_implementation {
    struct SanityEngine : SanityEngineT<SanityEngine, implementation::SanityEngine> {};
} // namespace winrt::sanity::factory_implementation
