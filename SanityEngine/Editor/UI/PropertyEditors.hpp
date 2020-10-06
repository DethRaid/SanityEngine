#pragma once

#include "core/types.hpp"
#include "core/concepts/EnumLike.hpp"
#include "rx/core/string.h"
#include "magic_enum.hpp"
#include "adapters/rex/rex_wrapper.hpp"

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

        auto enum_names_vector = Rx::Vector<Rx::String>{};
        enum_names_vector.reserve(enum_names.size());
        for(const auto& enum_name : enum_names) {
            enum_names_vector.emplace_back(RX_SYSTEM_ALLOCATOR, enum_name.data(), enum_name.length());
        }
    	
        selected_idx = draw_enum_property(enum_names_vector, selected_idx);

    	return magic_enum::enum_value<EnumType>(selected_idx);
    }
} // namespace Sanity::Editor::UI
