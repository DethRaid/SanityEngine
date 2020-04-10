#pragma once

class ID3D12Object;

namespace rx {
    struct string;
}

void set_object_name(ID3D12Object* object, const rx::string& name);
