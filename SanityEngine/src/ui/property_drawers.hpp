#pragma once

#include "glm/fwd.hpp"

namespace Rx {
    struct String;
}

namespace sanity::engine::ui {
    void vec3_property(const Rx::String& label, glm::vec3& vec);

	void quat_property(const Rx::String& label, glm::quat& quat);
}
