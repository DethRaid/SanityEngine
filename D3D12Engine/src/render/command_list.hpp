#pragma once

#ifdef interface
#undef interface
#endif

#include <rx/core/concepts/interface.h>
#include <rx/core/function.h>

namespace render {
    /*!
     * \brief Generic implementation of a command list
     */
    class CommandList : rx::concepts::interface {
    public:
        /*!
         * \brief Adds a function to this command list, to be execute when the command list has finished executing on the GPU
         */
        virtual void add_completion_function(rx::function<void()> completion_func) = 0;
    };
} // namespace render
