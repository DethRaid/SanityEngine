module;

#include <wrl/client.h>

#include "rx/core/types.h"
#include "rx/math/mat4x4.h"
#include "rx/math/vec2.h"
#include "rx/math/vec3.h"
#include "rx/math/vec4.h"

export module types;

export using Microsoft::WRL::ComPtr;

export using Rx::Byte;
export using Rx::Size;

export using Rx::Uint8;
export using Rx::Uint16;
export using Rx::Uint32;
export using Rx::Uint64;

export using Int8 = Rx::Sint8;
export using Int16 = Rx::Sint16;
export using Int32 = Rx::Sint32;
export using Int64 = Rx::Sint64;

export using Rx::Float32;
export using Rx::Float64;

export using Rx::Math::Vec2f;
export using Rx::Math::Vec2i;
export using Uint2 = Rx::Math::Vec2<Uint32>;
export using Double2 = Rx::Math::Vec2<Float64>;

export using Rx::Math::Vec3f;
export using Rx::Math::Vec3i;
export using Uint3 = Rx::Math::Vec3<Uint32>;
export using Double3 = Rx::Math::Vec3<Float64>;

export using Rx::Math::Vec4f;
export using Rx::Math::Vec4i;
export using Uint4 = Rx::Math::Vec4<Uint32>;
export using Double4 = Rx::Math::Vec4<Float64>;

export using Rx::Math::Mat4x4f;

export using Rx::operator""_z;
export using Rx::operator""_u8;
export using Rx::operator""_u16;
export using Rx::operator""_u32;
export using Rx::operator""_u64;
