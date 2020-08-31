#ifndef RX_CONSOLE_INTERFACE_H
#define RX_CONSOLE_INTERFACE_H
#include "rx/console/variable.h"
#include "rx/console/command.h"

namespace Rx::Console {

struct Token;

struct Interface {
  static bool load(const char* _file_name);
  static bool save(const char* _file_name);

  static VariableReference* add_variable(VariableReference* _reference);
  static void add_command(const String& _name, const char* _signature,
    Function<bool(const Vector<Command::Argument>&)>&& _function);

  static VariableReference* find_variable_by_name(const String& _name);
  static VariableReference* find_variable_by_name(const char* _name);

  static bool execute(const String& _contents);

  template<typename... Ts>
  static void print(const char* _format, Ts&&... _arguments);
  static void write(const String& _message);
  static void clear();
  static const Vector<String>& lines();

  static Vector<String> auto_complete_variables(const String& _prefix);
  static Vector<String> auto_complete_commands(const String& _prefix);

private:
  // set variable |_reference| with token |_token|
  static VariableStatus set_from_reference_and_token(VariableReference* _reference, const Token& _token);

  // set variable |_reference| with value |_value|
  template<typename T>
  static VariableStatus set_from_reference_and_value(VariableReference* _reference, const T& _value);

  // merge-sort variable references in alphabetical order
  static VariableReference* split(VariableReference* _reference);
  static VariableReference* merge(VariableReference* _lhs, VariableReference* _rhs);
  static VariableReference* sort(VariableReference* _reference);
};

template<typename... Ts>
inline void Interface::print(const char* _format, Ts&&... _arguments) {
  write(String::format(_format, Utility::forward<Ts>(_arguments)...));
}

inline VariableReference* Interface::find_variable_by_name(const String& _name) {
  return find_variable_by_name(_name.data());
}

} // namespace rx::console

#endif // RX_CONSOLE_INTERFACE_H
