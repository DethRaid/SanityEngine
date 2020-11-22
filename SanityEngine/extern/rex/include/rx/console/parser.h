#ifndef RX_CONSOLE_PARSER_H
#define RX_CONSOLE_PARSER_H

#include "rx/math/vec2.h"
#include "rx/math/vec3.h"
#include "rx/math/vec4.h"

#include "rx/core/assert.h"
#include "rx/core/string.h"

#include "rx/core/utility/construct.h"
#include "rx/core/utility/destruct.h"

namespace Rx::Console {

struct Token {
  RX_MARK_NO_COPY(Token);

  enum class Type {
    k_atom,
    k_string,
    k_boolean,
    k_int,
    k_float,
    k_vec4f,
    k_vec4i,
    k_vec3f,
    k_vec3i,
    k_vec2f,
    k_vec2i,
  };

  Token(Type _type, String&& value_);
  Token(Token&& token_);
  constexpr Token(bool _value);
  constexpr Token(Sint32 _value);
  constexpr Token(Float32 _value);
  constexpr Token(const Math::Vec4f& _value);
  constexpr Token(const Math::Vec4i& _value);
  constexpr Token(const Math::Vec3f& _value);
  constexpr Token(const Math::Vec3i& _value);
  constexpr Token(const Math::Vec2f& _value);
  constexpr Token(const Math::Vec2i& _value);
  ~Token();

  Token& operator=(Token&& token_);

  constexpr Type kind() const;

  const String& as_atom() const &;
  const String& as_string() const &;
  bool as_boolean() const;
  Sint32 as_int() const;
  Float32 as_float() const;
  const Math::Vec4f& as_vec4f() const &;
  const Math::Vec4i& as_vec4i() const &;
  const Math::Vec3f& as_vec3f() const &;
  const Math::Vec3i& as_vec3i() const &;
  const Math::Vec2f& as_vec2f() const &;
  const Math::Vec2i& as_vec2i() const &;

  String print() const;

private:
  void destroy();
  void assign(Token&& token_);

  Type m_type;

  union {
    String m_as_atom;
    String m_as_string;
    bool m_as_boolean;
    Sint32 m_as_int;
    Float32 m_as_float;
    Math::Vec4f m_as_vec4f;
    Math::Vec4i m_as_vec4i;
    Math::Vec3f m_as_vec3f;
    Math::Vec3i m_as_vec3i;
    Math::Vec2f m_as_vec2f;
    Math::Vec2i m_as_vec2i;
  };
};

inline constexpr Token::Token(bool _value)
  : m_type{Type::k_boolean}
  , m_as_boolean{_value}
{
}

inline constexpr Token::Token(Sint32 _value)
  : m_type{Type::k_int}
  , m_as_int{_value}
{
}

inline constexpr Token::Token(Float32 _value)
  : m_type{Type::k_float}
  , m_as_float{_value}
{
}

inline constexpr Token::Token(const Math::Vec4f& _value)
  : m_type{Type::k_vec4f}
  , m_as_vec4f{_value}
{
}

inline constexpr Token::Token(const Math::Vec4i& _value)
  : m_type{Type::k_vec4i}
  , m_as_vec4i{_value}
{
}

inline constexpr Token::Token(const Math::Vec3f& _value)
  : m_type{Type::k_vec3f}
  , m_as_vec3f{_value}
{
}

inline constexpr Token::Token(const Math::Vec3i& _value)
  : m_type{Type::k_vec3i}
  , m_as_vec3i{_value}
{
}

inline constexpr Token::Token(const Math::Vec2f& _value)
  : m_type{Type::k_vec2f}
  , m_as_vec2f{_value}
{
}

inline constexpr Token::Token(const Math::Vec2i& _value)
  : m_type{Type::k_vec2i}
  , m_as_vec2i{_value}
{
}

inline constexpr Token::Type Token::kind() const {
  return m_type;
}

inline const String& Token::as_atom() const & {
  RX_ASSERT(m_type == Type::k_atom, "invalid Type");
  return m_as_atom;
}

inline const String& Token::as_string() const & {
  RX_ASSERT(m_type == Type::k_string, "invalid Type");
  return m_as_string;
}

inline bool Token::as_boolean() const {
  RX_ASSERT(m_type == Type::k_boolean, "invalid Type");
  return m_as_boolean;
}

inline Sint32 Token::as_int() const {
  RX_ASSERT(m_type == Type::k_int, "invalid Type");
  return m_as_int;
}

inline Float32 Token::as_float() const {
  RX_ASSERT(m_type == Type::k_float, "invalid Type");
  return m_as_float;
}

inline const Math::Vec4f& Token::as_vec4f() const & {
  RX_ASSERT(m_type == Type::k_vec4f, "invalid Type");
  return m_as_vec4f;
}

inline const Math::Vec4i& Token::as_vec4i() const & {
  RX_ASSERT(m_type == Type::k_vec4i, "invalid Type");
  return m_as_vec4i;
}

inline const Math::Vec3f& Token::as_vec3f() const & {
  RX_ASSERT(m_type == Type::k_vec3f, "invalid Type");
  return m_as_vec3f;
}

inline const Math::Vec3i& Token::as_vec3i() const & {
  RX_ASSERT(m_type == Type::k_vec3i, "invalid Type");
  return m_as_vec3i;
}

inline const Math::Vec2f& Token::as_vec2f() const & {
  RX_ASSERT(m_type == Type::k_vec2f, "invalid Type");
  return m_as_vec2f;
}

inline const Math::Vec2i& Token::as_vec2i() const & {
  RX_ASSERT(m_type == Type::k_vec2i, "invalid Type");
  return m_as_vec2i;
}

struct Parser {
  RX_MARK_NO_COPY(Parser);
  RX_MARK_NO_MOVE(Parser);

  struct Diagnostic {
    Diagnostic(Memory::Allocator& _allocator);
    String message;
    Size offset;
    Size length;
    bool inside = false;
    bool caret;
  };

  Parser(Memory::Allocator& _allocator);

  bool parse(const String& _contents);

  const Diagnostic& error() const &;
  Vector<Token>&& tokens();

  constexpr Memory::Allocator& allocator() const;

private:
  bool parse_int(const char*& contents_, Sint32& value_);
  bool parse_float(const char*& contents_, Float32& value_);
  void consume_spaces();
  void record_span();

  template<typename... Ts>
  bool error(bool _caret, const char* _format, Ts&&... _arguments);

  Memory::Allocator& m_allocator;
  Vector<Token> m_tokens;
  Diagnostic m_diagnostic;

  const char* m_ch;
  const char* m_first;
};

inline Parser::Diagnostic::Diagnostic(Memory::Allocator& _allocator)
  : message{_allocator}
  , offset{0}
  , length{0}
  , caret{false}
{
}

inline const Parser::Diagnostic& Parser::error() const & {
  return m_diagnostic;
}

inline Vector<Token>&& Parser::tokens() {
  return Utility::move(m_tokens);
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Parser::allocator() const {
  return m_allocator;
}

template<typename... Ts>
inline bool Parser::error(bool _caret, const char* _format, Ts&&... _arguments) {
  record_span();
  m_diagnostic.caret = _caret;
  if constexpr(sizeof...(Ts) != 0) {
    m_diagnostic.message =
      String::format(allocator(), _format, Utility::forward<Ts>(_arguments)...);
  } else {
    m_diagnostic.message = _format;
  }
  return false;
}

const char* token_type_as_string(Token::Type _type);

} // namespace rx::console

#endif // RX_CONSOLE_PARSER_H
