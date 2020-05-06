#pragma once

#include <utility>

template <typename FunctionType>
class Defer {
public:
    explicit Defer(FunctionType&& function_in) : function{std::move(function_in)} {};

    Defer(const Defer& other) = delete;
    Defer& operator=(const Defer& other) = delete;

    Defer(Defer&& old) noexcept = delete;
    Defer& operator=(Defer&& old) noexcept = delete;

    ~Defer() { function(); }

private:
    FunctionType function;
};

#define DEFER(varname, function) const auto varname = Defer{std::move(function)};
