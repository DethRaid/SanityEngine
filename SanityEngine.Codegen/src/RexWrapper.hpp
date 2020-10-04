#pragma once

#include "nlohmann/json.hpp"
#include "rx/core/memory/system_allocator.h"
#include "rx/core/vector.h"

namespace Rx {
    struct String;
}

using nlohmann::json;

namespace rex {
    class Wrapper {
    public:
        Wrapper();

        Wrapper(const Wrapper& other) = delete;
        Wrapper& operator=(const Wrapper& other) = delete;

        Wrapper(Wrapper&& old) noexcept = default;
        Wrapper& operator=(Wrapper&& old) noexcept = default;

        ~Wrapper();

    private:
        inline static bool initialized{false};
    };
} // namespace rex

#define RX_SYSTEM_ALLOCATOR Rx::Memory::SystemAllocator::instance()

constexpr bool RX_ITERATION_CONTINUE = true;
constexpr bool RX_ITERATION_STOP = false;

namespace Rx {
	// ReSharper disable CppInconsistentNaming
	void to_json(json& j, const String& entry);

	void from_json(const json& j, String& entry);

    template <typename ValueType>
    void to_json(json& j, const Vector<ValueType>& vector) {
        vector.each_fwd([&](const ValueType& element) {
            const auto element_json = json{element};
            j.push_back(element);
        });
    }

    template <typename ValueType>
    void from_json(const json& j, Vector<ValueType>& vector) {
        vector.reserve(j.size());
        for(const auto& element : j) {
            vector.push_back(element.get<ValueType>());
        }
    }
    // ReSharper restore CppInconsistentNaming
} // namespace Rx
