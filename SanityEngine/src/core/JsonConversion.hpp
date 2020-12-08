#pragma once

#include "nlohmann/json.hpp"

// Macros to simplify conversion from/to types
// from https://github.com/nlohmann/json/blob/fd7a9f600712b2724463e9f7f703878ade676d6e/include/nlohmann/detail/macro_scope.hpp#L141
// At the time of this file's creation, the nlohmann::json in vcpkg was version 3.8. The current version is 3.9.1

#define NLOHMANN_FROM_JSON(Type, ...)                                                                                                      \
    void from_json(const nlohmann::json& nlohmann_json_j, Type& nlohmann_json_t) {                                                         \
        NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(NLOHMANN_JSON_FROM, __VA_ARGS__))                                                         \
    }

#define SANITY_DEFINE_COMPONENT_JSON_CONVERSIONS(Type, ...)                                                                                \
    inline void to_json(nlohmann::json& nlohmann_json_j, const Type& nlohmann_json_t) {                                                    \
        NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(NLOHMANN_JSON_TO, __VA_ARGS__))                                                           \
        nlohmann_json_j["_class_id"] = _uuidof(Type);                                                                                      \
    }                                                                                                                                      \
    inline NLOHMANN_FROM_JSON(Type, __VA_ARGS__)
