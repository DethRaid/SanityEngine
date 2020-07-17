#include "core_clr_host.hpp"

#include <filesystem>

#include <nethost.h>

#include "Tracy.hpp"
#include "core/errors.hpp"
#include "core/types.hpp"
#include "rx/core/abort.h"
#include "rx/core/log.h"
#include "rx/core/string.h"

// Based on https://github.com/novelrt/NovelRT/blob/master/src/NovelRT/DotNet/RuntimeService.cpp

namespace coreclr {
    RX_LOG("\033[35;47mCoreCLR\033[0m", logger);

    Host::Host(const Rx::String& coreclr_working_directory) {
        ZoneScoped;

        char_t buffer[MAX_PATH];
        auto buffer_size = static_cast<size_t>(MAX_PATH);
        auto result = get_hostfxr_path(buffer, &buffer_size, nullptr);
        if(result != 0) {
            Rx::abort("Could not find HostFXR");
        }

        hostfxr = LoadLibraryExW(reinterpret_cast<LPCWSTR>(buffer), nullptr, 0);
        if(hostfxr == nullptr) {
            Rx::abort("Could not load HostFXR assembly at '%s'", Rx::WideString{reinterpret_cast<Uint16*>(buffer)});
        }

        logger->verbose("HostFXR assembly loaded");

        hostfxr_initialize_func = reinterpret_cast<hostfxr_initialize_for_runtime_config_fn>(
            GetProcAddress(hostfxr, "hostfxr_initialize_for_runtime_config"));
        hostfxr_create_delegate_func = reinterpret_cast<hostfxr_get_runtime_delegate_fn>(
            GetProcAddress(hostfxr, "hostfxr_get_runtime_delegate"));
        hostfxr_shutdown_func = reinterpret_cast<hostfxr_close_fn>(GetProcAddress(hostfxr, "hostfxr_close"));

        if(hostfxr_initialize_func == nullptr) {
            Rx::abort("Could not load HostFXR initialize function");
        }

        if(hostfxr_create_delegate_func == nullptr) {
            Rx::abort("Could not load HostFXR create delegate function");
        }

        if(hostfxr_shutdown_func == nullptr) {
            Rx::abort("Could not load HostFXR shutdown function");
        }

        const auto* runtime_config_path = L"E:/Documents/SanityEngine/SanityEngine.NET/SanityEngine.NET.runtimeconfig.json";

        result = hostfxr_initialize_func(runtime_config_path, nullptr, &host_context);
        if(result != 0) {
            Rx::abort("Could not initialize the HostFXR context");
        }

        result = hostfxr_create_delegate_func(host_context,
                                              hdt_load_assembly_and_get_function_pointer,
                                              reinterpret_cast<void**>(&hostfxr_load_assembly_and_get_function_pointer_func));
        if(result != 0 || hostfxr_load_assembly_and_get_function_pointer_func == nullptr) {
            hostfxr_shutdown_func(host_context);
            Rx::abort("Could not load the function to load an assembly and get a function pointer from it");
        }

        logger->info("Initialized HostFXR");
    }

    Host::~Host() {
        const auto result = hostfxr_shutdown_func(host_context);
        if(FAILED(result)) {
            logger->error("Could not shut down the CoreCLR host: %s", to_string(result));
        } else {
            logger->info("Shut down the CoreCLR host");
        }
    }

    void Host::load_assembly(const Rx::String& assembly_path) {
        // For now we hardcode a bunch of stuff. Eventually I'll figure out what I'm doing

        typedef void (*HiFunctionPtr)();
        HiFunctionPtr hi_function;

        const HRESULT
            result = hostfxr_load_assembly_and_get_function_pointer_func(L"SanityEngine.NET.dll",
                                                                         L"SanityEngine.EnvironmentObjectEditor, SanityEngine.NET",
                                                                         L"Hi",
                                                                         nullptr,
                                                                         nullptr,
                                                                         reinterpret_cast<void**>(&hi_function));
        if(FAILED(result)) {
            logger->error("Could not get a pointer to the Hi function: %s", to_string(result));
        } else {
            hi_function();
        }
    }
} // namespace coreclr
