#pragma once

#include "core/JsonConversion.hpp"
#include "core/components.hpp"

void to_json(nlohmann::json& j, const TransformComponent& c);

void from_json(const nlohmann::json& j, TransformComponent& c);
