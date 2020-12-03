#pragma once

#include <guiddef.h>

#include "nlohmann/json_fwd.hpp"

void to_json(nlohmann::json& j, const GUID& g);

void from_json(const nlohmann::json& j, GUID& g);
