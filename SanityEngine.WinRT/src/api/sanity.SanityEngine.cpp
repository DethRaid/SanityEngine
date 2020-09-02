#include "sanity.SanityEngine.hpp"

#include <winrt/Windows.UI.Xaml.Controls.h>

#include "globals.hpp"
#include "pch.h"
#include "sanity.SanityEngine.g.cpp"
#include "sanity_engine.hpp"

namespace winrt::sanity::implementation {
    SanityEngine::SanityEngine() {
        if(g_engine == nullptr) {
            create_engine();
        }
    }

    void SanityEngine::SetRenderSurface(Windows::UI::Xaml::Controls::SwapChainPanel const& renderSurface) {}

    void SanityEngine::Tick(double deltaTime) { throw hresult_not_implemented(); }

    void SanityEngine::create_engine() {
        Settings settings;

        g_engine = new ::SanityEngine{settings};
    }
} // namespace winrt::sanity::implementation
