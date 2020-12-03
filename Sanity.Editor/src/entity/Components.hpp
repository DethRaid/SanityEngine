#pragma once
#include <guiddef.h>
#include <rex/include/rx/core/vector.h>

namespace sanity::editor
{
    /*!
     * \brief A list of all the class IDs of the components for a given entity
     */
	struct ComponentClassIdList
	{
        Rx::Vector<GUID> class_ids;
	};
}
