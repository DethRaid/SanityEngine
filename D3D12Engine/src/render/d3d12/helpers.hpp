#pragma once

#include "../resources.hpp"
#include <dxgi.h>

class ID3D12Object;

namespace rx {
    struct string;
}

void set_object_name(ID3D12Object& object, const rx::string& name);

DXGI_FORMAT to_dxgi_format(render::ImageFormat format);
