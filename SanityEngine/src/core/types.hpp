#pragma once

#include <wrl/client.h>

#include "rx/core/types.h"
#include "rx/math/mat4x4.h"
#include "rx/math/vec2.h"
#include "rx/math/vec3.h"
#include "rx/math/vec4.h"

using Microsoft::WRL::ComPtr;

using Rx::Byte;
using Rx::Size;

using Rx::Uint8;
using Rx::Uint16;
using Rx::Uint32;
using Rx::Uint64;

using Int8 = Rx::Sint8;
using Int16 = Rx::Sint16;
using Int32 = Rx::Sint32;
using Int64 = Rx::Sint64;

using Rx::Float32;
using Rx::Float64;

using Rx::Math::Vec2f;
using Rx::Math::Vec2i;
using Uint2 = Rx::Math::Vec2<Uint32>;
using Double2 = Rx::Math::Vec2<Float64>;

using Rx::Math::Vec3f;
using Rx::Math::Vec3i;
using Uint3 = Rx::Math::Vec3<Uint32>;
using Double3 = Rx::Math::Vec3<Float64>;

using Rx::Math::Vec4f;
using Rx::Math::Vec4i;
using Uint4 = Rx::Math::Vec4<Uint32>;
using Double4 = Rx::Math::Vec4<Float64>;

using Rx::Math::Mat4x4f;

using Rx::operator""_z;
using Rx::operator""_u8;
using Rx::operator""_u16;
using Rx::operator""_u32;
using Rx::operator""_u64;
