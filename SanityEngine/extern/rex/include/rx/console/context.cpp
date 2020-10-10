#include <string.h> // strcmp
#include <stdlib.h> // strtoll, strtof
#include <errno.h> // errno, ERANGE

#include "rx/console/context.h"
#include "rx/console/variable.h"
#include "rx/console/command.h"
#include "rx/console/parser.h"

#include "rx/core/concurrency/spin_lock.h"
#include "rx/core/concurrency/scope_lock.h"

#include "rx/core/filesystem/file.h"

#include "rx/core/log.h" // RX_LOG

namespace Rx::Console {

static Concurrency::SpinLock g_lock;

static VariableReference* g_head RX_HINT_GUARDED_BY(g_lock);

RX_LOG("console", logger);

static GlobalGroup g_group_cvars{"console"};

static bool type_check(VariableType _VariableType, Token::Type _token_type) {
  switch (_VariableType) {
  case VariableType::k_boolean:
    return _token_type == Token::Type::k_boolean;
  case VariableType::k_string:
    return _token_type == Token::Type::k_string;
  case VariableType::k_int:
    return _token_type == Token::Type::k_int;
  case VariableType::k_float:
    return _token_type == Token::Type::k_float;
  case VariableType::k_vec4f:
    return _token_type == Token::Type::k_vec4f;
  case VariableType::k_vec4i:
    return _token_type == Token::Type::k_vec4i;
  case VariableType::k_vec3f:
    return _token_type == Token::Type::k_vec3f;
  case VariableType::k_vec3i:
    return _token_type == Token::Type::k_vec3i;
  case VariableType::k_vec2f:
    return _token_type == Token::Type::k_vec2f;
  case VariableType::k_vec2i:
    return _token_type == Token::Type::k_vec2i;
  }
  return false;
}

bool Context::write(const String& message_) {
  return m_lines.push_back(message_);
}

void Context::clear() {
  m_lines.clear();
}

const Vector<String>& Context::lines() {
  return m_lines;
}

Command* Context::add_command(const String& _name, const char* _signature,
  Function<bool(Context& console_, const Vector<Command::Argument>&)>&& _function)
{
  // Don't allow adding the same command multiple times.
  if (m_commands.find(_name)) {
    return nullptr;
  }

  return m_commands.insert(_name, {_name, _signature, Utility::move(_function)});
}

bool Context::execute(const String& _contents) {
  Parser parse{Memory::SystemAllocator::instance()};

  if (!parse.parse(_contents)) {
    const auto& diagnostic{parse.error()};

    print("^rerror: ^w%s", diagnostic.message);
    print("%s", _contents);

    String format;
    format += String::format("%*s^r", static_cast<int>(diagnostic.offset), "");
    if (!diagnostic.inside) {
      for (Size i = 0; i < diagnostic.length; i++) {
        format += '~';
      }
    }

    if (diagnostic.caret) {
      format += "^^";
    }

    print("%s", format);

    return false;
  }

  auto tokens{Utility::move(parse.tokens())};

  if (tokens.is_empty()) {
    return false;
  }

  if (tokens[0].kind() != Token::Type::k_atom) {
    print("^rerror: ^wexpected atom");
    return false;
  }

  const auto& atom = tokens[0].as_atom();
  if (auto* variable = find_variable_by_name(atom)) {
    if (tokens.size() > 1) {
      switch (set_from_reference_and_token(variable, tokens[1])) {
      case VariableStatus::k_success:
        print("^gsuccess: ^wChanged: \"%s\" to %s", atom, tokens[1].print());
        break;
      case VariableStatus::k_out_of_range:
        print("^rerror: ^wOut of range: \"%s\" has range %s", atom,
          variable->print_range());
        break;
      case VariableStatus::k_type_mismatch:
        print("^rerror: ^wType mismatch: \"%s\" expected %s, got %s", atom,
          VariableType_as_string(variable->type()),
          token_type_as_string(tokens[1].kind()));
        break;
      }
    } else {
      print("^cinfo: ^w%s = %s", atom, variable->print_current());
    }
  } else if (auto* command = m_commands.find(atom)) {
    tokens.erase(0, 1);
    command->execute_tokens(*this, tokens);
  } else {
    print("^rerror: ^wCommand or variable \"%s\", not found", atom);
  }

  return true;
}

Vector<String> Context::auto_complete_variables(const String& _prefix) {
  Vector<String> results;
  for (VariableReference* node = g_head; node; node = node->m_next) {
    if (!strncmp(node->name(), _prefix.data(), _prefix.size())) {
      results.push_back(node->name());
    }
  }
  return results;
}

Vector<String> Context::auto_complete_commands(const String& _prefix) {
  Vector<String> results;
  m_commands.each_key([&](const String& _key) {
    if (!strncmp(_key.data(), _prefix.data(), _prefix.size())) {
      results.push_back(_key);
    }
  });
  return results;
}

bool Context::load(const char* file_name) {
  // sort references
  {
    Concurrency::ScopeLock locked{g_lock};
    g_head = sort(g_head);
  }

  Filesystem::File file(file_name, "r");
  if (!file) {
    return false;
  }

  logger->info("loading '%s'", file_name);

  Parser parse{Memory::SystemAllocator::instance()};
  for (String line_contents; file.read_line(line_contents); ) {
    String line{line_contents.lstrip(" \t")};
    if (line.is_empty() || strchr("#;[", line[0])) {
      // ignore empty and comment lines
      continue;
    }

    if (!parse.parse(line_contents)) {
      logger->error("%s", parse.error().message);
    } else {
      auto tokens{Utility::move(parse.tokens())};

      if (tokens.size() < 2) {
        continue;
      }

      if (tokens[0].kind() != Token::Type::k_atom) {
        continue;
      }

      const auto& atom{tokens[0].as_atom()};
      if (auto* variable{find_variable_by_name(atom)}) {
        set_from_reference_and_token(variable, tokens[1]);
      } else {
        logger->error("'%s' not found", atom);
      }
    }
  }

  return true;
}

bool Context::save(const char* file_name) {
  Filesystem::File file{file_name, "w"};
  if (!file) {
    return false;
  }

  logger->info("saving '%s'", file_name);
  for (const VariableReference *head = g_head; head; head = head->m_next) {
    if (VariableType_is_ranged(head->type())) {
      file.print("## %s (in range %s, defaults to %s)\n",
        head->description(), head->print_range(), head->print_initial());
      file.print(head->is_initial() ? ";%s %s\n" : "%s %s\n",
        head->name(), head->print_current());
    } else {
      file.print("## %s (defaults to %s)\n",
        head->description(), head->print_initial());
      file.print(head->is_initial() ? ";%s %s\n" : "%s %s\n",
        head->name(), head->print_current());
    }
  }

  return true;
}

template<typename T>
VariableStatus Context::set_from_reference_and_value(VariableReference* _reference, const T& _value) {
  if (auto* cast{_reference->try_cast<T>()}) {
    return cast->set(_value);
  } else {
    return VariableStatus::k_type_mismatch;
  }
}

template VariableStatus Context::set_from_reference_and_value<bool>(VariableReference* _reference, const bool& _value);
template VariableStatus Context::set_from_reference_and_value<String>(VariableReference* _reference, const String& _value);
template VariableStatus Context::set_from_reference_and_value<Sint32>(VariableReference* _reference, const Sint32& _value);
template VariableStatus Context::set_from_reference_and_value<Float32>(VariableReference* _reference, const Float32& _value);
template VariableStatus Context::set_from_reference_and_value<Math::Vec4f>(VariableReference* _reference, const Math::Vec4f& _value);
template VariableStatus Context::set_from_reference_and_value<Math::Vec4i>(VariableReference* _reference, const Math::Vec4i& _value);
template VariableStatus Context::set_from_reference_and_value<Math::Vec3f>(VariableReference* _reference, const Math::Vec3f& _value);
template VariableStatus Context::set_from_reference_and_value<Math::Vec3i>(VariableReference* _reference, const Math::Vec3i& _value);
template VariableStatus Context::set_from_reference_and_value<Math::Vec2f>(VariableReference* _reference, const Math::Vec2f& _value);
template VariableStatus Context::set_from_reference_and_value<Math::Vec2i>(VariableReference* _reference, const Math::Vec2i& _value);

VariableStatus Context::set_from_reference_and_token(VariableReference* reference_, const Token& _token) {
  if (!type_check(reference_->type(), _token.kind())) {
    return VariableStatus::k_type_mismatch;
  }

  switch (reference_->type()) {
  case VariableType::k_boolean:
    return reference_->cast<bool>()->set(_token.as_boolean());
  case VariableType::k_string:
    return reference_->cast<String>()->set(_token.as_string());
  case VariableType::k_int:
    return reference_->cast<Sint32>()->set(_token.as_int());
  case VariableType::k_float:
    return reference_->cast<Float32>()->set(_token.as_float());
  case VariableType::k_vec4f:
    return reference_->cast<Math::Vec4f>()->set(_token.as_vec4f());
  case VariableType::k_vec4i:
    return reference_->cast<Math::Vec4i>()->set(_token.as_vec4i());
  case VariableType::k_vec3f:
    return reference_->cast<Math::Vec3f>()->set(_token.as_vec3f());
  case VariableType::k_vec3i:
    return reference_->cast<Math::Vec3i>()->set(_token.as_vec3i());
  case VariableType::k_vec2f:
    return reference_->cast<Math::Vec2f>()->set(_token.as_vec2f());
  case VariableType::k_vec2i:
    return reference_->cast<Math::Vec2i>()->set(_token.as_vec2i());
  }

  RX_HINT_UNREACHABLE();
}

VariableReference* Context::find_variable_by_name(const char* _name) {
  for (VariableReference* head{g_head}; head; head = head->m_next) {
    if (!strcmp(head->name(), _name)) {
      return head;
    }
  }
  return nullptr;
}

VariableReference* Context::add_variable(VariableReference* reference) {
  logger->info("registered '%s'", reference->m_name);
  Concurrency::ScopeLock locked(g_lock);
  VariableReference* next = g_head;
  g_head = reference;
  return next;
}

VariableReference* Context::split(VariableReference* reference) {
  if (!reference || !reference->m_next) {
    return nullptr;
  }
  VariableReference* splitted = reference->m_next;
  reference->m_next = splitted->m_next;
  splitted->m_next = split(splitted->m_next);
  return splitted;
}

VariableReference* Context::merge(VariableReference* lhs, VariableReference* rhs) {
  if (!lhs) {
    return rhs;
  }

  if (!rhs) {
    return lhs;
  }

  if (strcmp(lhs->m_name, rhs->m_name) > 0) {
    rhs->m_next = merge(lhs, rhs->m_next);
    return rhs;
  }

  lhs->m_next = merge(lhs->m_next, rhs);
  return lhs;
}

VariableReference* Context::sort(VariableReference* reference) {
  if (!reference) {
    return nullptr;
  }

  if (!reference->m_next) {
    return reference;
  }

  VariableReference* splitted = split(reference);

  return merge(sort(reference), sort(splitted));
}

} // namespace Rx::Console
