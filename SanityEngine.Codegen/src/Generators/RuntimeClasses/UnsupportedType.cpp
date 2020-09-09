#include "UnsupportedType.hpp"

UnsupportedType::UnsupportedType(const Rx::String& type_name)
    : std::domain_error{nullptr}, message{Rx::String::format("Unknown type %s", type_name)} {}

const char* UnsupportedType::what() const { return message.data(); }
