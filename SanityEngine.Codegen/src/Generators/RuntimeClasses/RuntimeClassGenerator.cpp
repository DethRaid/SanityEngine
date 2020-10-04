#include "RuntimeClassGenerator.hpp"

#include "Generators/RuntimeClasses/UnsupportedType.hpp"
#include "cppast/cpp_class.hpp"
#include "cppast/cpp_entity_kind.hpp"
#include "cppast/cpp_member_function.hpp"
#include "cppast/cpp_member_variable.hpp"
#include "cppast/cpp_namespace.hpp"
#include "cppast/libclang_parser.hpp"
#include "cppast/visitor.hpp"
#include "rx/core/filesystem/directory.h"
#include "rx/core/log.h"
#include "rx/core/map.h"
#include "rx/core/time/stop_watch.h"

RX_LOG("RuntimeClassGenerator", logger);

struct RuntimeClassGeneratorState {
    cppast::libclang_compile_config config;

    type_safe::object_ref<cppast::libclang_parser> parser;
};

void VisitDirectory(Rx::Filesystem::Directory& cpp_input_directory, RuntimeClassGeneratorState& state);

Rx::Optional<Rx::String> GenerateRuntimeClass(const cppast::cpp_class& class_entity,
                                                const Rx::String& runtime_class_output_filename,
                                                RuntimeClassGeneratorState& state);

Rx::Optional<Rx::String> GenerateBindingsForFile(RuntimeClassGeneratorState& state, const Rx::String& filepath);

/*!
 * \brief Converts from C++ type to C# 9 type
 */
Rx::String ToString(cppast::cpp_builtin_type_kind builtin_type_kind);

Rx::String ToString(cppast::cpp_entity_kind kind);

void GenerateCSharpBindings(const Rx::String& cpp_input_path, const Rx::String& c_sharp_output_directory) {
    cppast::libclang_compile_config config;
    config.set_flags(cppast::cpp_standard::cpp_1z);

    // TODO: Relative directories
    config.add_include_dir(R"(E:\Documents\SanityEngine\SanityEngine\extern\physx\include)");
    config.add_include_dir(R"(E:\Documents\SanityEngine\SanityEngine\extern\physx\include\physx)");
    config.add_include_dir(R"(E:\Documents\SanityEngine\SanityEngine\extern\rex\include)");
    config.add_include_dir(R"(E:\Documents\SanityEngine\SanityEngine\extern\tracy)");
    config.add_include_dir(R"(E:\Documents\SanityEngine\SanityEngine\extern\json5\include)");
    config.add_include_dir(R"(E:\Documents\SanityEngine\SanityEngine\extern\dotnet\include)");
    config.add_include_dir(R"(E:\Documents\SanityEngine\SanityEngine\extern\D3D12MemoryAllocator)");
    config.add_include_dir(R"(E:\Documents\SanityEngine\SanityEngine\extern\bve\include)");
    config.add_include_dir(R"(E:\Documents\SanityEngine\SanityEngine\extern\pix\include)");
    config.add_include_dir(R"(E:\Documents\SanityEngine\SanityEngine\extern)");
    config.add_include_dir(R"(E:\Documents\SanityEngine\SanityEngine\src)");
    config.add_include_dir(R"(E:\Documents\SanityEngine\vcpkg_installed\x64-windows\include)");
    config.add_include_dir(R"(E:\Documents\SanityEngine\SanityEngine\src)");

    config.define_macro("WIN32", {});
    config.define_macro("_WINDOWS", {});
    config.define_macro("TRACY_ENABLE", {});
    config.define_macro("RX_DEBUG", {});
    config.define_macro("GLM_ENABLE_EXPERIMENTAL", {});
    config.define_macro("_CRT_SECURE_NO_WARNINGS", {});
    config.define_macro("GLM_FORCE_LEFT_HANDED", {});
    config.define_macro("NOMINMAX", {});
    config.define_macro("WIN32_LEAN_AND_MEAN", {});
    config.define_macro("GLFW_DLL", {});
    config.define_macro("CMAKE_INTDIR", "Debug");

    cppast::stderr_diagnostic_logger logger;
    cppast::libclang_parser parser{type_safe::ref(logger)};

    // TODO: Should I specify include directories? Or can I only care about the runtime classes?

    auto cpp_input_directory = Rx::Filesystem::Directory{cpp_input_path};
    auto state = RuntimeClassGeneratorState{
        .config = config,
        .parser = type_safe::ref(parser),
    };
    VisitDirectory(cpp_input_directory, state);
}

void VisitDirectory(Rx::Filesystem::Directory& directory, RuntimeClassGeneratorState& state) {
    auto directory_stopwatch = Rx::Time::StopWatch{};
    directory_stopwatch.start();

    directory.each([&](Rx::Filesystem::Directory::Item&& item) {
        if(item.is_directory()) {
            auto new_directory = item.as_directory();
            VisitDirectory(*new_directory, state);

        } else if(item.is_file()) {
            const auto filename = item.name();
            if(filename.ends_with(".hpp")) {
                auto file_stopwatch = Rx::Time::StopWatch{};
                file_stopwatch.start();

                const auto filepath = Rx::String::format("%s/%s", item.directory().path(), filename);

                try {
                    GenerateBindingsForFile(state, filepath);
                }
                catch(const cppast::libclang_error& e) {
                    logger->error("Error generating code from file %s: %s", filename, e.what());
                }

                file_stopwatch.stop();
                logger->info("Visited file %s in %s", filename, file_stopwatch.elapsed());
            }
        }
    });

    directory_stopwatch.stop();
    logger->info("Visited directory %s in %s", directory.path(), directory_stopwatch.elapsed());
}

Rx::Optional<Rx::String> GenerateBindingsForFile(RuntimeClassGeneratorState& state, const Rx::String& filepath) {
    const cppast::cpp_entity_index index;
    const auto file = state.parser->parse(index, filepath.data(), state.config);
    if(state.parser->error()) {
        logger->error("Could not parse .hpp file %s", filepath);
        return Rx::nullopt;

    } else {
        // First pass: Collect all the runtime classes
        cppast::visit(*file, [&](const cppast::cpp_entity& entity, cppast::visitor_info info) {
            logger->info("Entity %s is of type %s", entity.name().c_str(), ToString(entity.kind()));

            const auto is_class = entity.kind() == cppast::cpp_entity_kind::class_t;

            const auto has_runtimeclass_attribute = std::find_if(entity.attributes().begin(),
                                                                 entity.attributes().end(),
                                                                 [&](const cppast::cpp_attribute& attribute) {
                                                                     return attribute.name() == "sanity::runtimeclass";
                                                                 }) != entity.attributes().end();
            if(is_class && has_runtimeclass_attribute) {
                const auto extensionless_filename = filepath.substring(0, filepath.size() - 4);
                const auto idl_filename = Rx::String::format("%s.cs", extensionless_filename);
                GenerateRuntimeClass(static_cast<const cppast::cpp_class&>(entity), idl_filename, state);
            }
        });

        // Second pass: Write them to file
    }

	return Rx::nullopt;
}

Rx::Optional<Rx::String> GenerateRuntimeClass(const cppast::cpp_class& class_entity,
                                                const Rx::String& runtime_class_output_filename,
                                                RuntimeClassGeneratorState& state) {
    const auto class_name = Rx::String{class_entity.name().c_str()};

    const auto class_string = Rx::String::format("namespace Sanity\n{\n    public class %s \n    {", class_name);

    // Format params: return type, method name, class name, method arguments
    const auto* const method_format_string = "private static extern %s %s(%s* self, %s);";

    class_entity.parent().map([&](const cppast::cpp_entity& entity) {
        const auto namespace_name = Rx::String{entity.name().data()};
        // The parent is _probably_ a namespace? Let's figure it out
        if(entity.kind() == cppast::cpp_entity_kind::namespace_t) {
            logger->info("C++ class %s is in namespace %s", class_name, namespace_name);

            bool in_public_section = false;

            for(const auto& child_entity : class_entity) {
                // Poke the state machine to respect the access modifier
                if(child_entity.kind() == cppast::cpp_entity_kind::access_specifier_t) {
                    const auto& access_specifier_entity = static_cast<const cppast::cpp_access_specifier&>(child_entity);
                    if(access_specifier_entity.access_specifier() == cppast::cpp_access_specifier_kind::cpp_public) {
                        in_public_section = true;

                    } else {
                        in_public_section = false;
                    }

                } else if(in_public_section) {
                    // Only generate members in the runtime classes for public members in the C++
                    if(child_entity.kind() == cppast::cpp_entity_kind::member_function_t) {
                        auto member_function_string = Rx::String{};
                        const auto& member_function = static_cast<const cppast::cpp_member_function&>(child_entity);

                        auto signature_string = Rx::String{member_function.signature().c_str()};
                        if(signature_string.ends_with(" const")) {
                            signature_string = signature_string.substring(0, signature_string.size() - 6);
                        }
                        auto parameters_string = signature_string.substring(1, signature_string.size() - 2);
                        const auto& parameters = parameters_string.split(',');

                        std::string return_type_string = "<unknown-type>";

                        switch(member_function.return_type().kind()) {
                            case cppast::cpp_type_kind::builtin_t: {
                                const auto& return_type = static_cast<const cppast::cpp_builtin_type&>(member_function.return_type());
                                return_type_string = to_string(return_type);
                            } break;
                            case cppast::cpp_type_kind::user_defined_t:
                                [[fallthrough]];
                            case cppast::cpp_type_kind::auto_t:
                                [[fallthrough]];
                            case cppast::cpp_type_kind::decltype_t:
                                [[fallthrough]];
                            case cppast::cpp_type_kind::decltype_auto_t:
                                [[fallthrough]];
                            case cppast::cpp_type_kind::cv_qualified_t:
                                [[fallthrough]];
                            case cppast::cpp_type_kind::pointer_t:
                                [[fallthrough]];
                            case cppast::cpp_type_kind::reference_t:
                                [[fallthrough]];
                            case cppast::cpp_type_kind::array_t:
                                [[fallthrough]];
                            case cppast::cpp_type_kind::function_t:
                                [[fallthrough]];
                            case cppast::cpp_type_kind::member_function_t:
                                [[fallthrough]];
                            case cppast::cpp_type_kind::member_object_t:
                                [[fallthrough]];
                            case cppast::cpp_type_kind::template_parameter_t:
                                [[fallthrough]];
                            case cppast::cpp_type_kind::template_instantiation_t:
                                [[fallthrough]];
                            case cppast::cpp_type_kind::dependent_t:
                                [[fallthrough]];
                            case cppast::cpp_type_kind::unexposed_t:
                                break;
                        }

                        logger->info("Method '%s'\nParameters: %s\nReturn type: %s",
                                     signature_string,
                                     parameters_string,
                                     return_type_string.c_str());

                    } else if(child_entity.kind() == cppast::cpp_entity_kind::member_variable_t) {
                        // Turn member variables into properties
                    }
                }
            }
        }
    });

    return Rx::nullopt;
}

Rx::String ToString(const cppast::cpp_builtin_type_kind builtin_type_kind) {
    switch(builtin_type_kind) {
        case cppast::cpp_void:
            return "void";

        case cppast::cpp_bool:
            return "bool";

        case cppast::cpp_uchar:
            return "uint8";

        case cppast::cpp_ushort:
            return "uint16";

        case cppast::cpp_uint:
            [[fallthrough]];
        case cppast::cpp_ulong:
            return "uint";

        case cppast::cpp_ulonglong:
            return "ulong";

        case cppast::cpp_uint128:
            throw UnsupportedType{"__uint128"};

        case cppast::cpp_schar:
            return "int8";

        case cppast::cpp_short:
            return "int16";

        case cppast::cpp_int:
            [[fallthrough]];
        case cppast::cpp_long:
            return "int";

        case cppast::cpp_longlong:
            return "long";

        case cppast::cpp_int128:
            throw UnsupportedType{"__int128"};

        case cppast::cpp_float:
            return "float";

        case cppast::cpp_double:
            [[fallthrough]];
        case cppast::cpp_longdouble:
            return "double";

        case cppast::cpp_char:
            throw UnsupportedType{"char"};

        case cppast::cpp_wchar:
            [[fallthrough]];
        case cppast::cpp_char16:
            return "char";

        case cppast::cpp_char32:
            return "int";

        case cppast::cpp_nullptr:
            return "object";

        default:
            return "<unknown-type>";
    }
}

Rx::String ToString(const cppast::cpp_entity_kind kind) {
    switch(kind) {
        case cppast::cpp_entity_kind::file_t:
            return "file";

        case cppast::cpp_entity_kind::macro_parameter_t:
            return "macro_parameter";

        case cppast::cpp_entity_kind::macro_definition_t:
            return "macro_definition";

        case cppast::cpp_entity_kind::include_directive_t:
            return "include_directive";

        case cppast::cpp_entity_kind::language_linkage_t:
            return "language_linkage";

        case cppast::cpp_entity_kind::namespace_t:
            return "namespace";

        case cppast::cpp_entity_kind::namespace_alias_t:
            return "namespace_alias";

        case cppast::cpp_entity_kind::using_directive_t:
            return "using_directive";

        case cppast::cpp_entity_kind::using_declaration_t:
            return "using_declaration";

        case cppast::cpp_entity_kind::type_alias_t:
            return "type_alias";

        case cppast::cpp_entity_kind::enum_t:
            return "enum";

        case cppast::cpp_entity_kind::enum_value_t:
            return "enum_value";

        case cppast::cpp_entity_kind::class_t:
            return "class";

        case cppast::cpp_entity_kind::access_specifier_t:
            return "access_specifier";

        case cppast::cpp_entity_kind::base_class_t:
            return "base_class";

        case cppast::cpp_entity_kind::variable_t:
            return "variable";

        case cppast::cpp_entity_kind::member_variable_t:
            return "member_variable";

        case cppast::cpp_entity_kind::bitfield_t:
            return "bitfield";

        case cppast::cpp_entity_kind::function_parameter_t:
            return "function_parameter";

        case cppast::cpp_entity_kind::function_t:
            return "function";

        case cppast::cpp_entity_kind::member_function_t:
            return "member_function";

        case cppast::cpp_entity_kind::conversion_op_t:
            return "conversion_op";

        case cppast::cpp_entity_kind::constructor_t:
            return "constructor";

        case cppast::cpp_entity_kind::destructor_t:
            return "destructor";

        case cppast::cpp_entity_kind::friend_t:
            return "friend";

        case cppast::cpp_entity_kind::template_type_parameter_t:
            return "template_type_parameter";

        case cppast::cpp_entity_kind::non_type_template_parameter_t:
            return "non_type_template_parameter";

        case cppast::cpp_entity_kind::template_template_parameter_t:
            return "template_template_parameter";

        case cppast::cpp_entity_kind::alias_template_t:
            return "alias_template";

        case cppast::cpp_entity_kind::variable_template_t:
            return "variable_template";

        case cppast::cpp_entity_kind::function_template_t:
            return "function_template";

        case cppast::cpp_entity_kind::function_template_specialization_t:
            return "function_template_specialization";

        case cppast::cpp_entity_kind::class_template_t:
            return "class_template";

        case cppast::cpp_entity_kind::class_template_specialization_t:
            return "class_template_specialization";

        case cppast::cpp_entity_kind::static_assert_t:
            return "static_assert";

        case cppast::cpp_entity_kind::unexposed_t:
            return "unexposed";
    }

	return "<unknown-entity>";
}
