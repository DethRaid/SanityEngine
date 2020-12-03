#include "core/RexJsonConversion.hpp"

#include "nlohmann/json.hpp"

namespace Rx {
    void to_json(nlohmann::json& j, const String& s) { j = reinterpret_cast<const char*>(s.data()); }

    void from_json(const nlohmann::json& j, String& s) {
        const auto string = j.get<std::string>();
        s = string.c_str();
    }
} // namespace Rx
