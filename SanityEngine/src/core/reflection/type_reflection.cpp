#include "type_reflection.hpp"

namespace sanity::engine {
    Rx::String TypeReflection::get_name_of_type(const GUID type_id) const {
        if(const auto* type_name = type_names.find(type_id); type_name != nullptr) {
            return *type_name;
        }

        return "Unknown type";
    }

    const Rx::Map<GUID, Rx::String>& TypeReflection::get_type_names() const { return type_names; }
} // namespace sanity::engine
