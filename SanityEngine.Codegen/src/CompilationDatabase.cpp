#include "CompilationDatabase.hpp"

namespace Sanity::Codegen {
    // ReSharper disable CppInconsistentNaming
    void to_json(json& j, const CompilationDatabaseEntry& entry) {
        j["directory"] = entry.directory;
        j["file"] = entry.file;
        j["arguments"] = entry.arguments;
    }

    void from_json(const json& j, CompilationDatabaseEntry& entry) {
    	j["directory"].get_to(entry.directory);
        j["file"].get_to(entry.file);
        j["arguments"].get_to(entry.arguments);
    }


    // ReSharper restore CppInconsistentNaming
} // namespace Sanity::Codegen
