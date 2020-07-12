#pragma once

#include <Windows.h>

#include "core/types.hpp"
#include "coreclr_host_api.hpp"

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
         * \brief Handle to the CoreCLR assembly
         */
        HMODULE coreclr{nullptr};

        /*!
         * \brief Function in the CoreCLR assembly that initializes CoreCLR
         */
        coreclr_initialize_ptr coreclr_initialize_func{nullptr};
        coreclr_create_delegate_ptr coreclr_create_delegate_func{nullptr};
        coreclr_shutdown_ptr coreclr_shutdown_func{nullptr};

        /*!
         * \brief Handle to this CoreCLR host
         */
        HANDLE host_handle{nullptr};

        /*!
         * \brief Domain ID of this CoreCLR host
         */
        Uint32 domain_id{0};
    };
} // namespace coreclr
