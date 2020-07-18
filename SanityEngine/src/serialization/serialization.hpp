#pragma once

#include <combaseapi.h>

#include "core/errors.hpp"
#include "core/types.hpp"
#include "glm/glm.hpp"
#include "json5/json5.hpp"
#include "json5/json5_input.hpp"
#include "json5/json5_output.hpp"
#include "json5/json5_reflect.hpp"
#include "rx/core/hints/force_inline.h"
#include "rx/core/log.h"
#include "rx/core/string.h"

// Serializers for SanityEngine's dependencies

JSON5_CLASS(glm::vec3, x, y, z)
JSON5_CLASS(glm::vec2, x, y)

namespace json5::detail {
    RX_LOG("Json5", json5_logger);

    RX_HINT_FORCE_INLINE value write(writer& w, const GUID& guid) {
        // yolo
        // const auto guid_string = Rx::String::format("%X-%X-%X-%X",
        //                                            guid.Data1,
        //                                            guid.Data2,
        //                                            guid.Data3,
        //                                             Rx::String{reinterpret_cast<const char*>(guid.Data4), Size(8)});

        Rx::WideString wide_string;

        LPOLESTR guid_wide_string_array;

        const auto result = StringFromIID(guid, &guid_wide_string_array);
        if(FAILED(result)) {
            json5_logger->error("Could not serialize GUID: %s", ::to_string(result));
        }

        const auto guid_wide_string = Rx::WideString{reinterpret_cast<Uint16*>(guid_wide_string_array)};
        const auto guid_string = guid_wide_string.to_utf8();

        CoTaskMemFree(guid_wide_string_array);

        json5_logger->verbose("Serializing GUID %s", guid_string);

        return write(w, guid_string.data());
    }

    RX_HINT_FORCE_INLINE error read(const value& in, GUID& guid) {
        const auto guid_string = Rx::String{in.get_c_str("")};
        const auto wide_guid_string = guid_string.to_utf16();
        const auto result = IIDFromString(reinterpret_cast<LPCOLESTR>(wide_guid_string.data()), &guid);
        if(FAILED(result)) {
            json5_logger->error("Could not deserialize GUID: %s", ::to_string(result));
            return {error::syntax_error}; // ig
        }

        return {error::none};
    }
} // namespace json5::detail
