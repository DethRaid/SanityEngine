#pragma once

#include <unordered_set>

#include "glm/glm.hpp"
#include "json5/json5.hpp"
#include "json5/json5_input.hpp"
#include "json5/json5_output.hpp"
#include "json5/json5_reflect.hpp"

// Serializers for SanityEngine's dependencies

JSON5_CLASS(glm::vec3, x, y, z)
JSON5_CLASS(glm::vec2, x, y)

namespace json5::detail {
    inline json5::value write(writer& w, const std::unordered_set<std::string>& in) {
        w.push_array();
        for(const auto& str : in) {
            w += write(w, str);
        }
        return w.pop();
    }

    inline error read(const json5::value& in, std::unordered_set<std::string>& out) {
        const auto& array = json5::array_view{in};
        for(const auto& tag : array) {
            out.insert(tag.get_c_str(""));
        }

        return {error::none};
    }
} // namespace json5::detail
