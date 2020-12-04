#include "class_id.hpp"

#include "core/components.hpp"

namespace sanity::engine {
    Rx::String class_name_from_guid(const GUID& class_id) {
        if(class_id == _uuidof(TransformComponent)) {
            return "TransformComponent";
        	
        } else if(class_id == _uuidof(SanityEngineEntity)) {
            return "SanityEngineEntity";
        }

    	return "Unknown class";
    }
} // namespace sanity::engine
