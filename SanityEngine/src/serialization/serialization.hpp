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
        w.push_object();

        wchar_t guid_wide_string[64] = {0};

        const auto result = StringFromIID(guid, reinterpret_cast<LPOLESTR*>(guid_wide_string));
        if(FAILED(result)) {
            json5_logger->error("Could not serialize GUID: %s", ::to_string(result));
            return w.pop();
        }

        const auto guid_string = Rx::WideString{reinterpret_cast<Uint16*>(guid_wide_string)}.to_utf8();
        w += write(w, guid_string.data());

        return w.pop();
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

    RX_HINT_FORCE_INLINE value write(writer& w, const Uint32& value) {
        w.push_object();
        w += write(w, static_cast<uint32_t>(value));
        return w.pop();
    }

    RX_HINT_FORCE_INLINE error read(const value& in, Uint32& value) { return read(in, static_cast<uint32_t&>(value)); }
} // namespace json5::detail
