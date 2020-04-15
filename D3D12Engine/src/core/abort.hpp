#pragma once

namespace std {
    class string;
}

[[noreturn]] void critical_error(const std::string& message);
