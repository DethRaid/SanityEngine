#pragma once

#include <functional>

namespace render {
    /*!
     * \brief Generic implementation of a command list
     */
    class CommandList {
    public:
        virtual ~CommandList() = default;

        /*!
         * \brief Adds a function to this command list, to be execute when the command list has finished executing on the GPU
         */
        virtual void add_completion_function(std::function<void()> completion_func) = 0;
    };
} // namespace render
