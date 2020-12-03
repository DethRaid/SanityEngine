#pragma once

#include "nlohmann/json.hpp"
#include "rx/core/string.h"

namespace Rx {
    void to_json(nlohmann::json& j, const String& s);

    void from_json(const nlohmann::json& j, String& s);

    template <typename T>
    void to_json(nlohmann::json& j, const Vector<T>& s);

    template <typename T>
    void from_json(const nlohmann::json& j, Vector<T>& s);

    template <typename T>
    void to_json(nlohmann::json& j, const Vector<T>& s) {
        s.each_fwd([&](const T& item) { j.push_back(item); });
    }

    template <typename T>
    void from_json(const nlohmann::json& j, Vector<T>& s) {
        for(const auto& json_item : j) {
            T item;
            from_json(json_item, item);
            s.push_back(s);
        }
    }
} // namespace Rx
