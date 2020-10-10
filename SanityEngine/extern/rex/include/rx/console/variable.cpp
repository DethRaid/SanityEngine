#include "rx/console/variable.h"
#include "rx/console/context.h"

#include "rx/core/hints/unreachable.h"

namespace Rx::Console {

const char* VariableType_as_string(VariableType _type) {
  switch (_type) {
  case VariableType::k_boolean:
    return "bool";
  case VariableType::k_string:
    return "string";
  case VariableType::k_int:
    return "int";
  case VariableType::k_float:
    return "float";
  case VariableType::k_vec4f:
    return "vec4f";
  case VariableType::k_vec4i:
    return "vec4i";
  case VariableType::k_vec3f:
    return "vec3f";
  case VariableType::k_vec3i:
    return "vec3i";
  case VariableType::k_vec2f:
    return "vec2f";
  case VariableType::k_vec2i:
    return "vec2i";
  }
  RX_HINT_UNREACHABLE();
}

static String escape(const String& _contents) {
  String result{_contents.allocator()};
  result.reserve(_contents.size() * 4);
  for (Size i{0}; i < _contents.size(); i++) {
    switch (_contents[i]) {
    case '"':
      [[fallthrough]];
    case '\\':
      result += '\\';
      [[fallthrough]];
    default:
      result += _contents[i];
      break;
    }
  }
  return result;
};

VariableReference::VariableReference(const char* name,
  const char* description, void* handle, VariableType type)
  : m_name{name}
  , m_description{description}
  , m_handle{handle}
  , m_type{type}
{
  m_next = Context::add_variable(this);
}

void VariableReference::reset() {
  switch (m_type) {
  case VariableType::k_boolean:
    cast<bool>()->reset();
    break;
  case VariableType::k_string:
    cast<String>()->reset();
    break;
  case VariableType::k_int:
    cast<Sint32>()->reset();
    break;
  case VariableType::k_float:
    cast<Float32>()->reset();
    break;
  case VariableType::k_vec4f:
    cast<Math::Vec4f>()->reset();
    break;
  case VariableType::k_vec4i:
    cast<Math::Vec4i>()->reset();
    break;
  case VariableType::k_vec3f:
    cast<Math::Vec3f>()->reset();
    break;
  case VariableType::k_vec3i:
    cast<Math::Vec3i>()->reset();
    break;
  case VariableType::k_vec2f:
    cast<Math::Vec2f>()->reset();
    break;
  case VariableType::k_vec2i:
    cast<Math::Vec2i>()->reset();
    break;
  }
}

String VariableReference::print_current() const {
  switch (m_type) {
  case VariableType::k_boolean:
    return cast<bool>()->get() ? "true" : "false";
  case VariableType::k_string:
    return String::format("\"%s\"", escape(cast<String>()->get()));
  case VariableType::k_int:
    return String::format("%d", cast<Sint32>()->get());
  case VariableType::k_float:
    return String::format("%f", cast<Float32>()->get());
  case VariableType::k_vec4f:
    return String::format("%s", cast<Math::Vec4f>()->get());
  case VariableType::k_vec4i:
    return String::format("%s", cast<Math::Vec4i>()->get());
  case VariableType::k_vec3f:
    return String::format("%s", cast<Math::Vec3f>()->get());
  case VariableType::k_vec3i:
    return String::format("%s", cast<Math::Vec3i>()->get());
  case VariableType::k_vec2f:
    return String::format("%s", cast<Math::Vec2f>()->get());
  case VariableType::k_vec2i:
    return String::format("%s", cast<Math::Vec2i>()->get());
  }

  RX_HINT_UNREACHABLE();
}

String VariableReference::print_range() const {
  if (m_type == VariableType::k_int) {
    const auto handle{cast<Sint32>()};
    const auto min{handle->min()};
    const auto max{handle->max()};
    const auto min_fmt{min == k_int_min ? "-inf" : String::format("%d", min)};
    const auto max_fmt{max == k_int_max ? "+inf" : String::format("%d", max)};
    return String::format("[%s, %s]", min_fmt, max_fmt);
  } else if (m_type == VariableType::k_float) {
    const auto handle{cast<Float32>()};
    const auto min{handle->min()};
    const auto max{handle->max()};
    const auto min_fmt{min == k_float_min ? "-inf" : String::format("%f", min)};
    const auto max_fmt{max == k_float_max ? "+inf" : String::format("%f", max)};
    return String::format("[%s, %s]", min_fmt, max_fmt);
  } else if (m_type == VariableType::k_vec4f) {
    const auto handle{cast<Vec4f>()};
    const auto min{handle->min()};
    const auto max{handle->max()};

    String min_fmt;
    if (min.is_any(k_float_min)) {
      const auto min_x{min.x == k_float_min ? "-inf" : String::format("%f", min.x)};
      const auto min_y{min.y == k_float_min ? "-inf" : String::format("%f", min.y)};
      const auto min_z{min.z == k_float_min ? "-inf" : String::format("%f", min.z)};
      const auto min_w{min.w == k_float_min ? "-inf" : String::format("%f", min.w)};
      min_fmt = String::format("{%s, %s, %s, %s}", min_x, min_y, min_z, min_w);
    } else {
      min_fmt = String::format("%s", min);
    }

    String max_fmt;
    if (max.is_any(k_float_max)) {
      const auto max_x{max.x == k_float_max ? "+inf" : String::format("%f", max.x)};
      const auto max_y{max.y == k_float_max ? "+inf" : String::format("%f", max.y)};
      const auto max_z{max.z == k_float_max ? "+inf" : String::format("%f", max.z)};
      const auto max_w{max.w == k_float_max ? "+inf" : String::format("%f", max.w)};
      max_fmt = String::format("{%s, %s, %s, %s}", max_x, max_y, max_z, max_w);
    } else {
      max_fmt = String::format("%s", max);
    }

    return String::format("[%s, %s]", min_fmt, max_fmt);
  } else if (m_type == VariableType::k_vec4i) {
    const auto handle{cast<Vec4i>()};
    const auto min{handle->min()};
    const auto max{handle->max()};

    String min_fmt;
    if (min.is_any(k_int_min)) {
      const auto min_x{min.x == k_int_min ? "-inf" : String::format("%d", min.x)};
      const auto min_y{min.y == k_int_min ? "-inf" : String::format("%d", min.y)};
      const auto min_z{min.z == k_int_min ? "-inf" : String::format("%d", min.z)};
      const auto min_w{min.w == k_int_min ? "-inf" : String::format("%d", min.w)};
      min_fmt = String::format("{%s, %s, %s, %s}", min_x, min_y, min_z, min_w);
    } else {
      min_fmt = String::format("%s", min);
    }

    String max_fmt;
    if (max.is_any(k_int_max)) {
      const auto max_x{max.x == k_int_max ? "+inf" : String::format("%d", max.x)};
      const auto max_y{max.y == k_int_max ? "+inf" : String::format("%d", max.y)};
      const auto max_z{max.z == k_int_max ? "+inf" : String::format("%d", max.z)};
      const auto max_w{max.w == k_int_max ? "+inf" : String::format("%d", max.w)};
      max_fmt = String::format("{%s, %s, %s, %s}", max_x, max_y, max_z, max_w);
    } else {
      max_fmt = String::format("%s", max);
    }

    return String::format("[%s, %s]", min_fmt, max_fmt);
  } else if (m_type == VariableType::k_vec3f) {
    const auto handle{cast<Vec3f>()};
    const auto min{handle->min()};
    const auto max{handle->max()};

    String min_fmt;
    if (min.is_any(k_float_min)) {
      const auto min_x{min.x == k_float_min ? "-inf" : String::format("%f", min.x)};
      const auto min_y{min.y == k_float_min ? "-inf" : String::format("%f", min.y)};
      const auto min_z{min.z == k_float_min ? "-inf" : String::format("%f", min.z)};
      min_fmt = String::format("{%s, %s, %s}", min_x, min_y, min_z);
    } else {
      min_fmt = String::format("%s", min);
    }

    String max_fmt;
    if (max.is_any(k_float_max)) {
      const auto max_x{max.x == k_float_max ? "+inf" : String::format("%f", max.x)};
      const auto max_y{max.y == k_float_max ? "+inf" : String::format("%f", max.y)};
      const auto max_z{max.z == k_float_max ? "+inf" : String::format("%f", max.z)};
      max_fmt = String::format("{%s, %s, %s, %s}", max_x, max_y, max_z);
    } else {
      max_fmt = String::format("%s", max);
    }

    return String::format("[%s, %s]", min_fmt, max_fmt);
  } else if (m_type == VariableType::k_vec3i) {
    const auto handle{cast<Vec3i>()};
    const auto min{handle->min()};
    const auto max{handle->max()};

    String min_fmt;
    if (min.is_any(k_int_min)) {
      const auto min_x{min.x == k_int_min ? "-inf" : String::format("%d", min.x)};
      const auto min_y{min.y == k_int_min ? "-inf" : String::format("%d", min.y)};
      const auto min_z{min.z == k_int_min ? "-inf" : String::format("%d", min.z)};
      min_fmt = String::format("{%s, %s, %s}", min_x, min_y, min_z);
    } else {
      min_fmt = String::format("%s", min);
    }

    String max_fmt;
    if (max.is_any(k_int_max)) {
      const auto max_x{max.x == k_int_max ? "+inf" : String::format("%d", max.x)};
      const auto max_y{max.y == k_int_max ? "+inf" : String::format("%d", max.y)};
      const auto max_z{max.z == k_int_max ? "+inf" : String::format("%d", max.z)};
      max_fmt = String::format("{%s, %s, %s}", max_x, max_y, max_z);
    } else {
      max_fmt = String::format("%s", max);
    }

    return String::format("[%s, %s]", min_fmt, max_fmt);
  } else if (m_type == VariableType::k_vec2f) {
    const auto handle{cast<Vec2f>()};
    const auto min{handle->min()};
    const auto max{handle->max()};

    String min_fmt;
    if (min.is_any(k_float_min)) {
      const auto min_x{min.x == k_float_min ? "-inf" : String::format("%f", min.x)};
      const auto min_y{min.y == k_float_min ? "-inf" : String::format("%f", min.y)};
      min_fmt = String::format("{%s, %s}", min_x, min_y);
    } else {
      min_fmt = String::format("%s", min);
    }

    String max_fmt;
    if (max.is_any(k_float_max)) {
      const auto max_x{max.x == k_float_max ? "+inf" : String::format("%f", max.x)};
      const auto max_y{max.y == k_float_max ? "+inf" : String::format("%f", max.y)};
      max_fmt = String::format("{%s, %s}", max_x, max_y);
    } else {
      max_fmt = String::format("%s", max);
    }

    return String::format("[%s, %s]", min_fmt, max_fmt);
  } else if (m_type == VariableType::k_vec2i) {
    const auto handle{cast<Vec2i>()};
    const auto min{handle->min()};
    const auto max{handle->max()};

    String min_fmt;
    if (min.is_any(k_int_min)) {
      const auto min_x{min.x == k_int_min ? "-inf" : String::format("%d", min.x)};
      const auto min_y{min.y == k_int_min ? "-inf" : String::format("%d", min.y)};
      min_fmt = String::format("{%s, %s}", min_x, min_y);
    } else {
      min_fmt = String::format("%s", min);
    }

    String max_fmt;
    if (max.is_any(k_int_max)) {
      const auto max_x{max.x == k_int_max ? "+inf" : String::format("%d", max.x)};
      const auto max_y{max.y == k_int_max ? "+inf" : String::format("%d", max.y)};
      max_fmt = String::format("{%s, %s}", max_x, max_y);
    } else {
      max_fmt = String::format("%s", max);
    }

    return String::format("[%s, %s]", min_fmt, max_fmt);
  }

  RX_HINT_UNREACHABLE();
}

String VariableReference::print_initial() const {
  switch (m_type) {
  case VariableType::k_boolean:
    return cast<bool>()->initial() ? "true" : "false";
  case VariableType::k_string:
    return String::format("\"%s\"", escape(cast<String>()->initial()));
  case VariableType::k_int:
    return String::format("%d", cast<Sint32>()->initial());
  case VariableType::k_float:
    return String::format("%f", cast<Float32>()->initial());
  case VariableType::k_vec4f:
    return String::format("%s", cast<Math::Vec4f>()->initial());
  case VariableType::k_vec4i:
    return String::format("%s", cast<Math::Vec4i>()->initial());
  case VariableType::k_vec3f:
    return String::format("%s", cast<Math::Vec3f>()->initial());
  case VariableType::k_vec3i:
    return String::format("%s", cast<Math::Vec3i>()->initial());
  case VariableType::k_vec2f:
    return String::format("%s", cast<Math::Vec2f>()->initial());
  case VariableType::k_vec2i:
    return String::format("%s", cast<Math::Vec2i>()->initial());
  }

  RX_HINT_UNREACHABLE();
}

bool VariableReference::is_initial() const {
  switch (m_type) {
  case VariableType::k_boolean:
    return cast<bool>()->get() == cast<bool>()->initial();
  case VariableType::k_string:
    return cast<String>()->get() == cast<String>()->initial();
  case VariableType::k_int:
    return cast<Sint32>()->get() == cast<Sint32>()->initial();
  case VariableType::k_float:
    return cast<Float32>()->get() == cast<Float32>()->initial();
  case VariableType::k_vec4f:
    return cast<Math::Vec4f>()->get() == cast<Math::Vec4f>()->initial();
  case VariableType::k_vec4i:
    return cast<Math::Vec4i>()->get() == cast<Math::Vec4i>()->initial();
  case VariableType::k_vec3f:
    return cast<Math::Vec3f>()->get() == cast<Math::Vec3f>()->initial();
  case VariableType::k_vec3i:
    return cast<Math::Vec3i>()->get() == cast<Math::Vec3i>()->initial();
  case VariableType::k_vec2f:
    return cast<Math::Vec2f>()->get() == cast<Math::Vec2f>()->initial();
  case VariableType::k_vec2i:
    return cast<Math::Vec2i>()->get() == cast<Math::Vec2i>()->initial();
  }

  RX_HINT_UNREACHABLE();
}

template struct Variable<Sint32>;
template struct Variable<Float32>;
template struct Variable<Vec2i>;
template struct Variable<Vec2f>;
template struct Variable<Vec3i>;
template struct Variable<Vec3f>;
template struct Variable<Vec4i>;
template struct Variable<Vec4f>;

} // namespace rx::console
