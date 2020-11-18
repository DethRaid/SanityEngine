#pragma once

#include <stdexcept>

#include "rx/core/rex_string.h"

class UnsupportedType : public std::domain_error {
public:
    explicit UnsupportedType(const Rx::String& type_name);

    const char* what() const override;

private:
    Rx::String message;
};
