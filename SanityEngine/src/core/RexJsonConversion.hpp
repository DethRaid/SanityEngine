#pragma once

#include "nlohmann/json.hpp"
#include "rx/core/map.h"
#include "rx/core/optional.h"
#include "rx/core/string.h"

namespace Rx {
    void to_json(nlohmann::json& j, const String& s);

    void from_json(const nlohmann::json& j, String& s);

    template <typename T>
    void to_json(nlohmann::json& j, const Optional<T>& o);

    template <typename T>
    void from_json(const nlohmann::json& j, Optional<T>& o);

    template <typename T>
    void to_json(nlohmann::json& j, const Vector<T>& v);

    template <typename T>
    void from_json(const nlohmann::json& j, Vector<T>& v);

    template <typename ValueType>
    void to_json(nlohmann::json& j, const Map<String, ValueType>& m);

    template <typename ValueType>
    void from_json(const nlohmann::json& j, Map<String, ValueType>& m);

    template <typename T>
    void to_json(nlohmann::json& j, const Optional<T>& o) {
        if(o) {
            j = *o;
        } else {
            j = nullptr;
        }
    }

    template <typename T>
    void from_json(const nlohmann::json& j, Optional<T>& o) {
        if(j.is_null()) {
            o = nullopt;
        } else {
            o = j.get<T>();
        }
    }

    template <typename T>
    void to_json(nlohmann::json& j, const Vector<T>& v) {
        v.each_fwd([&](const T& item) { j.push_back(item); });
    }

    template <typename T>
    void from_json(const nlohmann::json& j, Vector<T>& v) {
        for(const auto& json_item : j) {
            T item;
            from_json(json_item, item);
            v.push_back(item);
        }
    }

    template <typename ValueType>
    void to_json(nlohmann::json& j, const Map<String, ValueType>& m) {
        m.each_pair([&](const String& key, const ValueType& value) { j[key.data()] = value; });
    }

    template <typename ValueType>
    void from_json(const nlohmann::json& j, Map<String, ValueType>& m) {
        for(auto it : j.items()) {
            auto value = it.value().get<ValueType>();
            m.insert(it.key().c_str(), value);
        }
    }
} // namespace Rx
