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

RX_LOG("RuntimeClassGenerator", logger);

struct RuntimeClassGeneratorState {
    cppast::libclang_compile_config config;

    type_safe::object_ref<cppast::libclang_parser> parser;
};

void visit_directory(Rx::Filesystem::Directory& cpp_input_directory, RuntimeClassGeneratorState& state);

void generate_runtime_class(const cppast::cpp_class& class_entity,
                            const Rx::String& runtime_class_output_filename,
                            RuntimeClassGeneratorState& state);

/*!
 * \brief Converts from C++ type to MIDL 3.0 type
 */
Rx::String to_string(cppast::cpp_builtin_type_kind builtin_type_kind);

void GenerateCSharpBindings(const Rx::String& cpp_input_path, const Rx::String& c_sharp_output_directory) {
    cppast::libclang_compile_config config;
    config.set_flags(cppast::cpp_standard::cpp_1z);

    cppast::stderr_diagnostic_logger logger;
    cppast::libclang_parser parser{type_safe::ref(logger)};

    // TODO: Should I specify include directories? Or can I only care about the runtime classes?

    auto cpp_input_directory = Rx::Filesystem::Directory{cpp_input_path};
    auto state = RuntimeClassGeneratorState{
        .config = config,
        .parser = type_safe::ref(parser),
    };
    visit_directory(cpp_input_directory, state);
}

void visit_directory(Rx::Filesystem::Directory& directory, RuntimeClassGeneratorState& state) {
    directory.each([&](Rx::Filesystem::Directory::Item&& item) {
        if(item.is_directory()) {
            auto new_directory = item.as_directory();
            visit_directory(*new_directory, state);
        } else {
            if(item.is_file()) {
                const auto filename = item.name();
                if(filename.ends_with(".hpp")) {
                    // We found a hpp file! Check if it has any runtime classes
                    cppast::cpp_entity_index index;
                    auto file = state.parser->parse(index, filename.data(), state.config);
                    if(state.parser->error()) {
                        logger->error("Could not parse .hpp file %s", filename);

                    } else {
                        // First pass: Collect all the runtime classes
                        cppast::visit(*file, [&](const cppast::cpp_entity& entity, cppast::visitor_info info) {
                            const auto is_class = entity.kind() == cppast::cpp_entity_kind::class_t;

                            const auto has_runtimeclass_attribute = std::find_if(entity.attributes().begin(),
                                                                                 entity.attributes().end(),
                                                                                 [&](const cppast::cpp_attribute& attribute) {
                                                                                     return attribute.name() == "sanity::runtimeclass";
                                                                                 }) != entity.attributes().end();
                            if(is_class && has_runtimeclass_attribute) {
                                const auto extensionless_filename = filename.substring(0, filename.size() - 4);
                                const auto idl_filename = Rx::String::format("%s.cs", extensionless_filename);
                                generate_runtime_class(static_cast<const cppast::cpp_class&>(entity), idl_filename, state);
                            }
                        });
                    }
                }
            }
        }
    });
}

void generate_runtime_class(const cppast::cpp_class& class_entity,
                            const Rx::String& runtime_class_output_filename,
                            RuntimeClassGeneratorState& state) {	
    const auto class_name = Rx::String{class_entity.name().c_str()};

    const auto class_string = Rx::String::format("namespace Sanity\n{\n    public class %s \n    {", class_name);

	// Format params: return type, method name, class name, method arguments
	const auto method_format_string = "private static extern %s %s(%s* self, %s);";

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
                    // Only generate members in the runtime classes fo rpublic members in the C++
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
                        case cppast::cpp_type_kind::user_defined_t: break;
                        case cppast::cpp_type_kind::auto_t: break;
                        case cppast::cpp_type_kind::decltype_t: break;
                        case cppast::cpp_type_kind::decltype_auto_t: break;
                        case cppast::cpp_type_kind::cv_qualified_t: break;
                        case cppast::cpp_type_kind::pointer_t: break;
                        case cppast::cpp_type_kind::reference_t: break;
                        case cppast::cpp_type_kind::array_t: break;
                        case cppast::cpp_type_kind::function_t: break;
                        case cppast::cpp_type_kind::member_function_t: break;
                        case cppast::cpp_type_kind::member_object_t: break;
                        case cppast::cpp_type_kind::template_parameter_t: break;
                        case cppast::cpp_type_kind::template_instantiation_t: break;
                        case cppast::cpp_type_kind::dependent_t: break;
                        case cppast::cpp_type_kind::unexposed_t: break;
                        default: ;
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
}

Rx::String to_string(const cppast::cpp_builtin_type_kind builtin_type_kind) {
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
