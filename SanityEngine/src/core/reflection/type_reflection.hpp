#pragma once

#include <activation.h>
#include <rx/core/map.h>
#include <rx/core/string.h>

namespace sanity::engine {
    class TypeReflection {
    public:
        template <typename Type>
        void register_type_name(const Rx::String& type_name);

        template <typename Type>
        [[nodiscard]] Rx::String get_name_of_type() const;

        [[nodiscard]] Rx::String get_name_of_type(GUID type_id) const;

        [[nodiscard]] const Rx::Map<GUID, Rx::String>& get_type_names() const;

    private:
        Rx::Map<GUID, Rx::String> type_names;
    };

    template <typename Type>
    void TypeReflection::register_type_name(const Rx::String& type_name) {
        const auto type_id = __uuidof(Type);
        type_names.insert(type_id, type_name);
    }

    template <typename Type>
    Rx::String TypeReflection::get_name_of_type() const {
        const auto type_id = __uuidof(Type);

        return get_name_of_type(type_id);
    }
} // namespace sanity::engine
