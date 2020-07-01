#pragma once

#include <rx/core/types.h>
#include <rx/math/vec2.h>
#include <rx/math/vec3.h>
#include <rx/math/vec4.h>
#include <rx/math/mat4x4.h>

using Byte = Rx::Byte;
using Size = Rx::Size;

using Uint8 = Rx::Uint8;
using Uint16 = Rx::Uint16;
using Uint32 = Rx::Uint32;
using Uint64 = Rx::Uint64;

using Int32 = Rx::Sint32;
using Int64 = Rx::Sint64;

using Float32 = Rx::Float32;

using Rx::Math::Vec2i;
using Rx::Math::Vec2f;
using Vec2u = Rx::Math::Vec2<Uint32>;

using Rx::Math::Vec3i;
using Rx::Math::Vec3f;
using Vec3u = Rx::Math::Vec3<Uint32>;

using Rx::Math::Vec4i;
using Rx::Math::Vec4f;
using Vec4u = Rx::Math::Vec4<Uint32>;

using Rx::Math::Mat4x4f;

using Rx::operator ""_z;
using Rx::operator ""_u8;
using Rx::operator ""_u16;
using Rx::operator ""_u32;
using Rx::operator ""_u64;
