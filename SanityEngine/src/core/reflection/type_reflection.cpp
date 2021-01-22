#include "type_reflection.hpp"

namespace sanity::engine {
    Rx::String TypeReflection::get_name_of_type(GUID type_id) const {
        if(const auto* type_name = type_names.find(type_id); type_name != nullptr) {
            return *type_name;
        }

        return "Unknown type";
    }
} // namespace sanity::engine
