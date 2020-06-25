#pragma once

namespace colors {
    constexpr const char* DEFAULT_CONSOLE_COLOR = "\033[37m";

#pragma region DRED colors
    constexpr const char* COMPLETED_BREADCRUMB = "\033[32m";
    constexpr const char* INCOMPLETE_BREADCRUMB = "\033[33m";
    constexpr const char* CONTEXT_LABEL = "\033[36m";
#pragma endregion
} // namespace colors
