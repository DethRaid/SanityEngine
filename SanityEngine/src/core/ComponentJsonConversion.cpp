#include "ComponentJsonConversion.hpp"

#include "WindowsJsonConversion.hpp"
#include "core/GlmJsonConversion.hpp"

void to_json(nlohmann::json& j, const TransformComponent& c) {
    const auto class_id = _uuidof(TransformComponent);
    j = nlohmann::json{{"_class_id", class_id}, {"location", c.location}, {"rotation", c.rotation}, {"scale", c.scale}};
}

void from_json(const nlohmann::json& j, TransformComponent& c) {
    j.at("location").get_to(c.location);
    j.at("rotation").get_to(c.rotation);
    j.at("scale").get_to(c.scale);
}
