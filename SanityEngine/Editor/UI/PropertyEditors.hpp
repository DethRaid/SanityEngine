#pragma once

#include "core/types.hpp"
#include "core/concepts/EnumLike.hpp"
#include "rx/core/string.h"
#include "magic_enum.hpp"

namespace Sanity::Editor::UI {

    template <EnumLike EnumType>
    EnumType draw_enum_property(EnumType selected_value);

    Uint32 draw_enum_property(const Rx::Vector<Rx::String>& enum_values, Uint32 selected_idx);
} // namespace Sanity::Editor::UI

namespace Sanity::Editor::UI {
    template <EnumLike EnumType>
    EnumType draw_enum_property(EnumType selected_value) {
        auto selected_idx = magic_enum::enum_integer(selected_value);        
        const auto enum_names = magic_enum::enum_names<EnumType>();
    	
        selected_idx = draw_enum_property(enum_names, selected_idx);

    	return magic_enum::enum_value<EnumType>(selected_idx);
    }
} // namespace Sanity::Editor::UI
