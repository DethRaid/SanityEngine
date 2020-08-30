#pragma once

#include "ImGUI.g.h"

namespace winrt::SanityEngine_WinRT::implementation {
    struct ImGUI : ImGUIT<ImGUI> {
        ImGUI() = default;

        int32_t DummyProperty();
        void DummyProperty(int32_t value);
    };
} // namespace winrt::SanityEngine_WinRT::implementation

namespace winrt::SanityEngine_WinRT::factory_implementation {
    struct ImGUI : ImGUIT<World, implementation::ImGUI> {};
} // namespace winrt::SanityEngine_WinRT::factory_implementation
