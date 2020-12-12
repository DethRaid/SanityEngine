#pragma once

#include "core/types.hpp"
#include "glm/fwd.hpp"
#include "rx/core/map.h"

namespace sanity {
	namespace engine {
		struct Transform;
	}
}

namespace Rx {
    struct String;
}

namespace sanity::engine::ui {
    void draw_property_editor(const Rx::String& label, bool& b);

    void draw_property_editor(const Rx::String& label, Float32& f);
	
    void draw_property_editor(const Rx::String& label, glm::vec3& vec);

    void draw_property_editor(const Rx::String& label, glm::quat& quat);

	void draw_property_editor(const Rx::String& label, Transform& transform);

    void draw_property_editor(const Rx::String& label, Rx::String& string);

    template <typename KeyType, typename ValueType>
    void draw_property_editor(const Rx::String& label, Rx::Map<KeyType, ValueType>& map);

    template <typename KeyType, typename ValueType>
    void draw_property_editor(const Rx::String& label, Rx::Map<KeyType, ValueType>& map) {
        // TODO
    }
} // namespace sanity::engine::ui
