#pragma once

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
