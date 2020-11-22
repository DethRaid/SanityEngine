#pragma once

#include "rx/core/string.h"
#include "rx/core/vector.h"

class StringBuilder {
public:
    template <typename Arg, typename... Args>
    StringBuilder& append(const char* format_string, Arg arg, Args&&... args);

    StringBuilder& append(const Rx::String& string);

    [[nodiscard]] Rx::String build() const;

private:
    Rx::Vector<Rx::String> parts;
};

template <typename Arg, typename... Args>
StringBuilder& StringBuilder::append(const char* format_string, Arg arg, Args&&... args) {
    const auto string = Rx::String::format(format_string, arg, Rx::Utility::forward<Args>(args)...);
    return append(string);
}
