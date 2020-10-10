#pragma once

#include <memory>

#include "sanity_engine.hpp"

namespace Sanity::Editor {
    class SanityEditor {
    public:
        explicit SanityEditor();
    private:
        std::unique_ptr<SanityEngine> sanity_engine;
    };
} // namespace Sanity::Editor
