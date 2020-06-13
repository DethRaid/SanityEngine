#ifndef RX_CONSOLE_COMMAND_H
#define RX_CONSOLE_COMMAND_H
#include "rx/console/variable.h"
#include "rx/console/parser.h"

#include "rx/core/hints/empty_bases.h"

#include "rx/core/utility/exchange.h"

namespace Rx::Console {

// The signature specification works like this
//  b -> boolean
//  s -> string
//  i -> int
//  f -> float
//  v -> vector, followed by 2, 3, 4 for # of components, followed by i or f.
struct RX_HINT_EMPTY_BASES Command
  : Concepts::NoCopy
{
  struct RX_HINT_EMPTY_BASES Argument
    : Concepts::NoCopy
  {
    explicit Argument(){}
    explicit Argument(bool _value);
    explicit Argument(const String& _value);
    explicit Argument(Sint32 _value);
    explicit Argument(Float32 _value);
    explicit Argument(const Math::Vec4f& _value);
    explicit Argument(const Math::Vec4i& _value);
    explicit Argument(const Math::Vec3f& _value);
    explicit Argument(const Math::Vec3i& _value);
    explicit Argument(const Math::Vec2f& _value);
    explicit Argument(const Math::Vec2i& _value);

    Argument(Argument&& _argument);

    ~Argument();

    VariableType type;
    union {
      bool as_boolean;
      String as_string;
      Sint32 as_int;
      Float32 as_float;
      Math::Vec4f as_vec4f;
      Math::Vec4i as_vec4i;
      Math::Vec3f as_vec3f;
      Math::Vec3i as_vec3i;
      Math::Vec2f as_vec2f;
      Math::Vec2i as_vec2i;
    };
  };

  using Delegate = Function<bool(const Vector<Argument>& _arguments)>;

  Command(Memory::Allocator& _allocator, const String& _name,
          const char* _signature, Delegate&& _function);
  Command(const String& _name, const char* _signature, Delegate&& _function);
  Command(Command&& command_);

  Command& operator=(Command&& command_);

  template<typename... Ts>
  bool execute_arguments(Ts&&... _arguments);

  bool execute_tokens(const Vector<Token>& _tokens);

  const String& name() const &;

  constexpr Memory::Allocator& allocator() const;

private:
  bool execute();

  Ref<Memory::Allocator> m_allocator;
  Delegate m_delegate;
  Vector<Argument> m_arguments;
  String m_declaration;
  String m_name;
  const char* m_signature;
  Size m_argument_count;
};

inline Command::Argument::Argument(bool _value)
  : type{VariableType::k_boolean}
{
  as_boolean = _value;
}

inline Command::Argument::Argument(const String& _value)
  : type{VariableType::k_string}
{
  Utility::construct<String>(&as_string, _value);
}

inline Command::Argument::Argument(Sint32 _value)
  : type{VariableType::k_int}
{
  as_int = _value;
}

inline Command::Argument::Argument(Float32 _value)
  : type{VariableType::k_float}
{
  as_float = _value;
}

inline Command::Argument::Argument(const Math::Vec4f& _value)
  : type{VariableType::k_vec4f}
{
  as_vec4f = _value;
}

inline Command::Argument::Argument(const Math::Vec4i& _value)
  : type{VariableType::k_vec4i}
{
  as_vec4i = _value;
}

inline Command::Argument::Argument(const Math::Vec3f& _value)
  : type{VariableType::k_vec3f}
{
  as_vec3f = _value;
}

inline Command::Argument::Argument(const Math::Vec3i& _value)
  : type{VariableType::k_vec3i}
{
  as_vec3i = _value;
}

inline Command::Argument::Argument(const Math::Vec2f& _value)
  : type{VariableType::k_vec2f}
{
  as_vec2f = _value;
}

inline Command::Argument::Argument(const Math::Vec2i& _value)
  : type{VariableType::k_vec2i}
{
  as_vec2i = _value;
}

inline Command::Command(const String& _name, const char* _signature, Delegate&& _function)
  : Command{Memory::SystemAllocator::instance(), _name, _signature, Utility::move(_function)}
{
}

inline Command::Command(Command&& command_)
  : m_allocator{command_.m_allocator}
  , m_delegate{Utility::move(command_.m_delegate)}
  , m_arguments{Utility::move(command_.m_arguments)}
  , m_declaration{Utility::move(command_.m_declaration)}
  , m_name{Utility::move(command_.m_name)}
  , m_signature{Utility::exchange(command_.m_signature, nullptr)}
  , m_argument_count{Utility::exchange(command_.m_argument_count, 0)}
{
}

inline Command& Command::operator=(Command&& command_) {
  RX_ASSERT(&command_ != this, "self assignment");

  m_allocator = command_.m_allocator;
  m_delegate = Utility::move(command_.m_delegate);
  m_arguments = Utility::move(command_.m_arguments);
  m_declaration = Utility::move(command_.m_declaration);
  m_name = Utility::move(command_.m_name);
  m_signature = Utility::exchange(command_.m_signature, nullptr);
  m_argument_count = Utility::exchange(command_.m_argument_count, 0);

  return *this;
}

template<typename... Ts>
inline bool Command::execute_arguments(Ts&&... _arguments) {
  m_arguments.clear();
  (m_arguments.emplace_back(_arguments), ...);
  return execute();
}

inline const String& Command::name() const & {
  return m_name;
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Command::allocator() const {
  return m_allocator;
}

} // namespace rx::console

#endif // RX_CONSOLE_COMMAND
