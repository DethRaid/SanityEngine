#pragma once

#include <Windows.h>

#include "core/types.hpp"

#include "hostfxr.h"
#include "coreclr_delegates.h"

namespace Rx {
    struct String;
}

namespace coreclr {
    /*!
     * \brief Class that wraps the CoreCLR runtime and provides beautiful functionality
     */
    class Host {
    public:
        /*!
         * \brief Initializes a CoreCLR host
         *
         * \param coreclr_working_directory Path to the folder with all the CoreCLR files themselves
         */
        explicit Host(const Rx::String& coreclr_working_directory);

        ~Host();

        /*!
         * \brief Loads an assembly into the CoreCLR host, allowing future code to use the types and functions from that assembly
         *
         * \param assembly_path Path to the assembly to load. Must be relative to the working directory
         */
        void load_assembly(const Rx::String& assembly_path);

    private:
        /*!
         * \brief Handle to the hostfxr assembly
         */
        HMODULE hostfxr{nullptr};

        /*!
         * \brief Function in the CoreCLR assembly that initializes CoreCLR
         */
        hostfxr_initialize_for_runtime_config_fn hostfxr_initialize_func{nullptr};
        hostfxr_get_runtime_delegate_fn hostfxr_create_delegate_func{nullptr};
        hostfxr_close_fn hostfxr_shutdown_func{nullptr};

        load_assembly_and_get_function_pointer_fn hostfxr_load_assembly_and_get_function_pointer_func{nullptr};

        /*!
         * \brief Handle to this HostFXR host
         */
        hostfxr_handle host_context{nullptr};
    };
} // namespace coreclr
