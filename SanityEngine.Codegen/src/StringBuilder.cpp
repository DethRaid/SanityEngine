#include "StringBuilder.hpp"

StringBuilder& StringBuilder::append(const Rx::String& string) {
    parts.push_back(string);
    return *this;
}

Rx::String StringBuilder::build() const {
    auto total_size = Rx::Size{0};
    parts.each_fwd([&](const Rx::String& string) { total_size += string.size(); });

    auto final_string = Rx::String{};
    final_string.resize(total_size + 1);

    auto* write_ptr = final_string.data();
    parts.each_fwd([&](const Rx::String& string) {
        memcpy(write_ptr, string.data(), string.size());
        write_ptr += string.size();
    });
    *write_ptr = '\0';

	return final_string;
}
