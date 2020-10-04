#include "RuntimeClassGenerator.hpp"

#include "cppast/code_generator.hpp"
#include "cppast/cpp_entity.hpp"
#include "cppast/cpp_type.hpp"
#include "nlohmann/json.hpp"

namespace Sanity::Codegen::Horus {
    cppast::visit_filter CSharpBindingsGenerator::EntityFilter(const cppast::cpp_entity& entity, cppast::cpp_access_specifier_kind access) {
        const auto is_class = entity.kind() == cppast::cpp_entity_kind::class_t;
        const auto has_horusclass_attribute = has_attribute(entity.attributes(), "sanity::horus") != type_safe::nullopt;
        if(is_class && has_horusclass_attribute) {
            return cppast::visit_filter::include;
        }

        return cppast::visit_filter::exclude;
    }

    void CSharpBindingsGenerator::GenerateForClass(const cppast::cpp_class& entity)
    {
    }
} // namespace Sanity::Codegen::Horus
