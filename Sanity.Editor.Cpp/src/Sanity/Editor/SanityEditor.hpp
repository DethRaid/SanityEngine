#pragma once

#include <memory>

#include "sanity_engine.hpp"

namespace Sanity::Editor {
    class SanityEditor {
    private:
        std::shared_ptr<sanity::SanityEngine> sanity_engine;
    };
} // namespace Sanity::Editor
