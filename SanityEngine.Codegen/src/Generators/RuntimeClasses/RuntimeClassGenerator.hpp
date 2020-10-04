#pragma once

#include <cppast/visitor.hpp>

#include "../../StringBuilder.hpp"
#include "cppast/code_generator.hpp"
#include "Generators/Generator.hpp"
#include "rx/core/types.h"

namespace Sanity::Codegen::Horus {
    /// <summary>
    /// Generates a C interface for the C# P/Invoke code to call
    /// </summary>
    class CSharpBindingsGenerator final : public Generator {
    public:
        static cppast::visit_filter EntityFilter(const cppast::cpp_entity& entity, cppast::cpp_access_specifier_kind access);

        void GenerateForClass(const cppast::cpp_class& entity) override;
    };
} // namespace Sanity::Codegen::Horus
