#pragma once

#include <functional>

namespace rhi {
    /*!
     * \brief Generic implementation of a command list
     */
    class CommandList {
    public:
        virtual ~CommandList() = default;

        /*!
         * \brief Allows you to set the debug name of this command list. This is helpful in a debugger like Nsight Graphics
         */
        virtual void set_debug_name(const std::string& name) = 0;

        /*!
         * \brief Adds a function to this command list, to be execute when the command list has finished executing on the GPU
         */
        virtual void add_completion_function(std::function<void()>&& completion_func) = 0;
    };
} // namespace render
