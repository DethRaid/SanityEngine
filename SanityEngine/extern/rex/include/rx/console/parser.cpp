#include <string.h> // strncmp
#include <stdlib.h> // strtoll, strtof
#include <errno.h> // errnop, ERANGE

#include "rx/console/parser.h"
#include "rx/console/variable.h"

#include "rx/core/hints/unreachable.h"

namespace Rx::Console {

const char* token_type_as_string(Token::Type _type) {
  switch (_type) {
  case Token::Type::k_atom:
    return "atom";
  case Token::Type::k_string:
    return "string";
  case Token::Type::k_boolean:
    return "boolean";
  case Token::Type::k_int:
    return "int";
  case Token::Type::k_float:
    return "float";
  case Token::Type::k_vec4f:
    return "vec4f";
  case Token::Type::k_vec4i:
    return "vec4i";
  case Token::Type::k_vec3f:
    return "vec3f";
  case Token::Type::k_vec3i:
    return "vec3i";
  case Token::Type::k_vec2f:
    return "vec2f";
  case Token::Type::k_vec2i:
    return "vec2i";
  }
  RX_HINT_UNREACHABLE();
}

Token::Token(Type _type, String&& value_)
  : m_type{_type}
{
  switch (m_type) {
  case Type::k_atom:
    Utility::construct<String>(&m_as_atom, Utility::move(value_));
    break;
  case Type::k_string:
    Utility::construct<String>(&m_as_string, Utility::move(value_));
    break;
  default:
    RX_ASSERT(0, "invalid Type");
    break;
  }
}

Token::Token(Token&& token_) {
  assign(Utility::move(token_));
}

Token::~Token() {
  destroy();
}

Token& Token::operator=(Token&& token_) {
  destroy();
  assign(Utility::move(token_));
  return *this;
}

void Token::assign(Token&& token_) {
  m_type = token_.m_type;

  switch (m_type) {
  case Type::k_atom:
    Utility::construct<String>(&m_as_atom, Utility::move(token_.m_as_atom));
    Utility::destruct<String>(&token_.m_as_atom);
    break;
  case Type::k_string:
    Utility::construct<String>(&m_as_string, Utility::move(token_.m_as_string));
    Utility::destruct<String>(&token_.m_as_string);
    break;
  case Type::k_boolean:
    m_as_boolean = token_.m_as_boolean;
    break;
  case Type::k_int:
    m_as_int = token_.m_as_int;
    break;
  case Type::k_float:
    m_as_float = token_.m_as_float;
    break;
  case Type::k_vec4f:
    m_as_vec4f = token_.m_as_vec4f;
    break;
  case Type::k_vec4i:
    m_as_vec4i = token_.m_as_vec4i;
    break;
  case Type::k_vec3f:
    m_as_vec3f = token_.m_as_vec3f;
    break;
  case Type::k_vec3i:
    m_as_vec3i = token_.m_as_vec3i;
    break;
  case Type::k_vec2f:
    m_as_vec2f = token_.m_as_vec2f;
    break;
  case Type::k_vec2i:
    m_as_vec2i = token_.m_as_vec2i;
    break;
  }
}

void Token::destroy() {
  switch (m_type) {
  case Type::k_string:
    Utility::destruct<String>(&m_as_string);
    break;
  case Type::k_atom:
    Utility::destruct<String>(&m_as_atom);
    break;
  default:
    break;
  }
}

String Token::print() const {
  switch (m_type) {
  case Type::k_atom:
    return m_as_atom;
  case Type::k_string:
    return String::format("\"%s\"", m_as_string);
  case Type::k_boolean:
    return m_as_boolean ? "true" : "false";
  case Type::k_int:
    return String::format("%d", m_as_int);
  case Type::k_float:
    return String::format("%f", m_as_float);
  case Type::k_vec4f:
    return String::format("%s", m_as_vec4f);
  case Type::k_vec4i:
    return String::format("%s", m_as_vec4i);
  case Type::k_vec3f:
    return String::format("%s", m_as_vec3f);
  case Type::k_vec3i:
    return String::format("%s", m_as_vec3i);
  case Type::k_vec2f:
    return String::format("%s", m_as_vec2f);
  case Type::k_vec2i:
    return String::format("%s", m_as_vec2i);
  }

  return "";
}

Parser::Parser(Memory::Allocator& _allocator)
  : m_allocator{_allocator}
  , m_tokens{allocator()}
  , m_diagnostic{allocator()}
  , m_ch{nullptr}
  , m_first{nullptr}
{
}

static bool is_space(int _ch) {
  return _ch == ' ' || _ch == '\t';
}

static bool is_sign(int _ch) {
  return _ch == '-' || _ch == '+';
}

static bool is_digit(int _ch) {
  return _ch >= '0' && _ch <= '9';
}

static bool is_identifier(int _ch) {
  return (_ch >= 'a' && _ch <= 'z') || (_ch >= 'A' && _ch <= 'Z') || _ch == '_';
}

static bool is_terminator(int _ch) {
  return is_space(_ch) || _ch == '\0';
}

static bool float_like(const char* _contents) {
  if (is_sign(*_contents)) {
    _contents++;
  }

  while (is_digit(*_contents)) {
    _contents++;
  }

  return *_contents == '.';
}

bool Parser::parse(const String& _contents) {
  m_tokens.clear();

  m_first = _contents.data();
  m_ch = m_first;

  m_diagnostic = {allocator()};

  while (*m_ch) {
    if (*m_ch == '\"') {
      m_ch++; // Skip '\"'
      record_span();
      String contents{allocator()};
      while (*m_ch && *m_ch != '\"') {
        if (*m_ch == '\\' && (m_ch[1] == '\"' || m_ch[1] == '\'')) {
          // NOTE(dweiler): This should not be escaped.
          contents += m_ch[1];
          m_ch += 2;
        } else {
          contents += *m_ch++;
        }
      }
      if (*m_ch != '\"') {
        return error(true, "expected closing '\"'");
      }
      m_ch++; // Skip '\"'
      m_tokens.emplace_back(Token::Type::k_string, Utility::move(contents));
      record_span();
    } else if (*m_ch == '{') {
      m_ch++; // Skip '{'
      consume_spaces();

      const bool is_float{float_like(m_ch)};

      Float32 fs[4];
      Sint32 is[4];

      Sint32 i{0};
      for (i = 0; i < 4; i++) {
        consume_spaces();
        record_span();

        if (*m_ch == '}') {
          return error(true, "expected value for vector.%c", "xyzw"[i]);
        } else if (!is_digit(*m_ch) && *m_ch != '.') {
          return error(true, "unexpected token '%c' in vector.%c", *m_ch, "xyzw"[i]);
        }

        if (is_float) {
          if (!float_like(m_ch) && parse_int(m_ch, is[i])) {
            return error(false, "expected float for vector.%c", "xyzw"[i]);
          } else if (!parse_float(m_ch, fs[i])) {
            return false;
          }
        } else {
          if (float_like(m_ch) && parse_float(m_ch, fs[i])) {
            return error(false, "expected int for vector.%c", "xyzw"[i]);
          } else if (!parse_int(m_ch, is[i])) {
            return false;
          }
        }

        consume_spaces();
        record_span();

        if (*m_ch != ',') {
          if (is_sign(*m_ch) || is_digit(*m_ch) || *m_ch == '.') {
            return error(true, "expected ','");
          }
          break;
        } else {
          m_ch++;
        }
      }

      i++;

      if (i > 4 && *m_ch != '}') {
        return error(false, "vector contains too many scalars");
      }

      // We're expecting a '}' to complete the vector.
      if (*m_ch != '}') {
        return error(true, "expected '}'");
      }

      if (i < 2) {
        return error(true, "vector contains too few scalars");
      }

      m_ch++; // Skip '}'

      switch (i) {
      case 2:
        is_float ? m_tokens.push_back(Math::Vec2f{fs[0], fs[1]})
                 : m_tokens.push_back(Math::Vec2i{is[0], is[1]});
        break;
      case 3:
        is_float ? m_tokens.push_back(Math::Vec3f{fs[0], fs[1], fs[2]})
                 : m_tokens.push_back(Math::Vec3i{is[0], is[1], is[2]});
        break;
      case 4:
        is_float ? m_tokens.push_back(Math::Vec4f{fs[0], fs[1], fs[2], fs[3]})
                 : m_tokens.push_back(Math::Vec4i{is[0], is[1], is[2], is[3]});
        break;
      }
    } else if (is_sign(*m_ch) || is_digit(*m_ch) || (*m_ch == '.' && is_digit(m_ch[1]))) {
      record_span();
      if (float_like(m_ch)) {
        if (Float32 value; parse_float(m_ch, value)) {
          m_tokens.emplace_back(value);
        } else {
          return false;
        }
      } else {
        if (Sint32 value; parse_int(m_ch, value)) {
          m_tokens.emplace_back(value);
        } else {
          return false;
        }
      }
      record_span();
    } else if (!strncmp(m_ch, "true", 4)) {
      m_tokens.emplace_back(true);
      m_ch += 4;
    } else if (!strncmp(m_ch, "false", 5)) {
      m_tokens.emplace_back(false);
      m_ch += 5;
    } else if (is_identifier(*m_ch)) {
      record_span();
      String contents{allocator()};
      while (is_identifier(*m_ch) || is_digit(*m_ch) || *m_ch == '.') {
        contents += *m_ch++;
      }
      m_tokens.emplace_back(Token::Type::k_atom, Utility::move(contents));
      record_span();
    }

    if (!is_terminator(*m_ch)) {
      return error(true, "unexpected token");
    } else if (is_space(*m_ch)) {
      m_ch++;
    } else {
      break;
    }
  }

  return true;
}

bool Parser::parse_int(const char*& contents_, Sint32& value_) {
  char* end;
  const long long value{strtoll(contents_, &end, 10)};
  contents_ = end;

  if (errno == ERANGE || value < k_int_min || value > k_int_max) {
    return error(false, "out of range for int");
  }

  value_ = static_cast<Sint32>(value);
  return true;
}

bool Parser::parse_float(const char*& contents_, Float32& value_) {
  char* end;
  const Float32 value{strtof(contents_, &end)};
  contents_ = end;

  if (errno == ERANGE) {
    return error(false, "out of range for float");
  }

  value_ = value;
  return true;
}

void Parser::consume_spaces() {
  while (is_space(*m_ch)) {
    m_ch++;
  }
}

void Parser::record_span() {
  RX_ASSERT(m_ch >= m_first, "parser broken");

  const auto offset{static_cast<Size>(m_ch - m_first)};
  if (m_diagnostic.inside) {
    m_diagnostic.length = offset > m_diagnostic.offset ? offset - m_diagnostic.offset : 0;
  } else {
    m_diagnostic.offset = offset;
  }
  m_diagnostic.inside = !m_diagnostic.inside;
}

} // namespace rx::console
