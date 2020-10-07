#include "CompilationDatabase.hpp"
#include "Generators/RuntimeClasses/RuntimeClassGenerator.hpp"
#include "RexWrapper.hpp"
#include "Types.hpp"
#include "cppast/cpp_entity.hpp"
#include "cppast/cpp_member_variable.hpp"
#include "cppast/cpp_namespace.hpp"
#include "cppast/libclang_parser.hpp"
#include "cppast/visitor.hpp"
#include "nlohmann/json.hpp"
#include "rx/core/filesystem/directory.h"
#include "rx/core/filesystem/file.h"
#include "rx/core/log.h"
#include "rx/core/map.h"
#include "rx/core/time/stop_watch.h"

RX_LOG("Sanity.Codegen", logger);

namespace Sanity::Codegen {
    /*!
     * \brief Converts from C++ type to C# 9 type
     */
    [[nodiscard]] Rx::String ToString(cppast::cpp_builtin_type_kind builtin_type_kind);

    [[nodiscard]] Rx::String ToString(cppast::cpp_entity_kind kind);

    [[nodiscard]] Rx::Vector<CompilationDatabaseEntry> CollectHeadersFromDirectory(Rx::Filesystem::Directory& dir) {
        const Rx::Vector<Rx::String> arguments = Rx::Array{
        	// Language options
        	"--std=c++1z",

        	// Include paths
        	R"(-IE:\Documents\SanityEngine\SanityEngine\extern\rex\include)",
            R"(-IE:\Documents\SanityEngine\SanityEngine\extern\physx\include)",
            R"(-IE:\Documents\SanityEngine\SanityEngine\extern\physx\include\physx)",
            R"(-IE:\Documents\SanityEngine\SanityEngine\extern\rex\include)",
            R"(-IE:\Documents\SanityEngine\SanityEngine\extern\tracy)",
            R"(-IE:\Documents\SanityEngine\SanityEngine\extern\json5\include)",
            R"(-IE:\Documents\SanityEngine\SanityEngine\extern\dotnet\include)",
            R"(-IE:\Documents\SanityEngine\SanityEngine\extern\D3D12MemoryAllocator)",
            R"(-IE:\Documents\SanityEngine\SanityEngine\extern\bve\include)",
            R"(-IE:\Documents\SanityEngine\SanityEngine\extern\pix\include)",
            R"(-IE:\Documents\SanityEngine\SanityEngine\extern)",
            R"(-IE:\Documents\SanityEngine\SanityEngine\src)",
            R"(-IE:\Documents\SanityEngine\vcpkg_installed\x64-windows\include)",
            R"(-IE:\Documents\SanityEngine\SanityEngine\src)",

        	// Global defines
            "-DWIN32",
            "-D_WINDOWS",
            "-DTRACY_ENABLE",
            "-DRX_DEBUG",
            "-DGLM_ENABLE_EXPERIMENTAL",
            "-D_CRT_SECURE_NO_WARNINGS",
            "-DGLM_FORCE_LEFT_HANDED",
            "-DNOMINMAX",
            "-DWIN32_LEAN_AND_MEAN",
            "-DGLFW_DLL",
            "-DCMAKE_INTDIR=Debug",
        };
    	
        Rx::Vector<CompilationDatabaseEntry> db_entries;
        dir.each([&](Rx::Filesystem::Directory::Item&& item) {
            if(item.is_directory()) {
                auto inner_dir = item.as_directory();
                const auto inner_db_entries = CollectHeadersFromDirectory(*inner_dir);
                db_entries.append(inner_db_entries);

            } else if(item.is_file() && item.name().ends_with(".hpp")) {
                db_entries.push_back(
                    CompilationDatabaseEntry{.directory = dir.path(), .file = item.name(), .arguments = arguments});
            }
        });

        return db_entries;
    }

    void RunCodegenForDirectory(const Rx::String& cpp_input_directory, const Rx::String& c_sharp_output_directory) {
        logger->info("Scanning directory %s for C++ header files", cpp_input_directory);

        auto dir = Rx::Filesystem::Directory{cpp_input_directory.data()};
        const auto db_entries = CollectHeadersFromDirectory(dir);
        const json compilation_database = db_entries;
        const auto compilation_database_string = compilation_database.dump();
        const auto compilation_database_filename = Rx::String::format("%s/compile_commands.json", cpp_input_directory);
        {
            auto compilation_database_file = Rx::Filesystem::File{compilation_database_filename, "w"};
            compilation_database_file.print(compilation_database_string.c_str());
            compilation_database_file.close();
        }

        logger->info("Parsing files in directory %s", cpp_input_directory);

        cppast::cpp_entity_index index;
        cppast::simple_file_parser<cppast::libclang_parser> parser{type_safe::ref(index)};

        cppast::libclang_compilation_database database{cpp_input_directory.data()};

        try {
            auto parse_timer = Rx::Time::StopWatch{};
            parse_timer.start();

            parse_database(parser, database);

            parse_timer.stop();

            logger->info("Parsed the files in directory %s in %s", cpp_input_directory, parse_timer.elapsed());
        }
        catch(cppast::libclang_error& ex) {
            logger->error(ex.what());
            return;
        }

        if(parser.error()) {
            // Non-fatal error. The parser already logged it to stderr, nothing for us to do
            logger->warning("Non-fatal error, that we're treating as fatal? Very strange");
            return;
        }

        logger->info("Beginning codegen phase");

        auto total_gen_timer = Rx::Time::StopWatch{};
        total_gen_timer.start();

        auto horus_class_generator = Horus::CSharpBindingsGenerator{};

        for(const auto& file : parser.files()) {
            auto file_codegen_timer = Rx::Time::StopWatch{};
            file_codegen_timer.start();

            cppast::visit(file,
                          Horus::CSharpBindingsGenerator::EntityFilter,
                          [&](const cppast::cpp_entity& entity, const cppast::visitor_info& info) {
                              if(info.is_old_entity()) {
                                  return;
                              }

                              horus_class_generator.GenerateForClass(static_cast<const cppast::cpp_class&>(entity));
                          });

            file_codegen_timer.stop();
            logger->info("Codegen for file %s completed in %s", file.name().c_str(), file_codegen_timer.elapsed());
        }

        total_gen_timer.stop();
        logger->info("Codegen phase completed in %s", total_gen_timer.elapsed());
    }

} // namespace Sanity::Codegen

/*!
 * \brief Scans all .hpp files in a provided directory
 *
 * Usage: `SanityEngine.Codegen <C++ dir> <C# dir>`
 */
int main(const Sint32 argc, const char** argv) {
    auto return_code = Sint32{0};
    auto rex_wrapper = rex::Wrapper{};

    logger->info("HELLO HUMAN");

    if(argc != 3) {
        logger->error("Wrong number of command-line parameters. Usage:\n\n\tSanityEngine.Codegen <C++ directory> <C# directory>");
        return_code = -1;

    } else {
        const auto cpp_directory = Rx::String{argv[1]};
        const auto c_sharp_directory = Rx::String{argv[2]};

        Sanity::Codegen::RunCodegenForDirectory(cpp_directory, c_sharp_directory);
    }

    logger->warning("REMAIN INDOORS");

    return return_code;
}
