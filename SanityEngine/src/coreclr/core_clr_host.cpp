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
    constexpr const char* INIT_FUNC_NAME = "hostfxr_initialize_for_runtime_config";
    constexpr const char* CLOSE_FUNC_NAME = "hostfxr_close";

    constexpr const char* GET_PROPERTY_FUNC_NAME = "hostfxr_get_runtime_property_value";
    constexpr const char* SET_PROPERTY_FUNC_NAME = "hostfxr_set_runtime_property_value";

    constexpr const char* GET_DELEGATE_FUNC_NAME = "hostfxr_get_runtime_delegate";

    constexpr const char_t* TPA_PROPERTY = L"TRUSTED_PLATFORM_ASSEMBLIES";

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

        load_hostfxr_functions(hostfxr);

        const auto* runtime_config_path = L"E:/Documents/SanityEngine/SanityEngine.NET/SanityEngine.NET.runtimeconfig.json";

        result = hostfxr_init(runtime_config_path, nullptr, &host_context);
        if(result != 0) {
            Rx::abort("Could not initialize the HostFXR context");
        }

        add_managed_assembly_to_tpa_list();

        void* func_ptr{nullptr};
        result = hostfxr_create_delegate(host_context, hdt_load_assembly_and_get_function_pointer, &func_ptr);
        if(result != 0 || func_ptr == nullptr) {
            hostfxr_close(host_context);
            Rx::abort("Could not load the function to load an assembly and get a function pointer from it");
        }
        hostfxr_load_assembly_and_get_function_pointer_func = load_assembly_and_get_function_pointer_fn(func_ptr);

        logger->info("Initialized HostFXR");
    }

    Host::~Host() {
        const auto result = hostfxr_close(host_context);
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

        const HRESULT result = hostfxr_load_assembly_and_get_function_pointer_func(
            L"E:\\Documents\\SanityEngine\\build\\SanityEngine\\Debug\\SanityEngine.NET.dll",
            L"SanityEngine.EnvironmentObjectEditor, SanityEngine.NET",
            L"Hi",
            L"System.Action, System.Private.Corelib",
            nullptr,
            reinterpret_cast<void**>(&hi_function));
        if(FAILED(result)) {
            logger->error("Could not get a pointer to the Hi function: %s", to_string(result));
        } else {
            hi_function();
        }
    }

    void Host::load_hostfxr_functions(const HMODULE hostfxr_module) {
        hostfxr_init = hostfxr_initialize_for_runtime_config_fn(GetProcAddress(hostfxr_module, INIT_FUNC_NAME));
        hostfxr_close = hostfxr_close_fn(GetProcAddress(hostfxr_module, CLOSE_FUNC_NAME));

        hostfxr_get_runtime_property_value = hostfxr_get_runtime_property_value_fn(GetProcAddress(hostfxr, GET_PROPERTY_FUNC_NAME));
        hostfxr_set_runtime_property_value = hostfxr_set_runtime_property_value_fn(GetProcAddress(hostfxr_module, SET_PROPERTY_FUNC_NAME));

        hostfxr_create_delegate = hostfxr_get_runtime_delegate_fn(GetProcAddress(hostfxr_module, GET_DELEGATE_FUNC_NAME));

        if(hostfxr_init == nullptr) {
            Rx::abort("Could not load HostFXR initialize function");
        }

        if(hostfxr_close == nullptr) {
            Rx::abort("Could not load HostFXR close function");
        }

        if(hostfxr_get_runtime_property_value == nullptr) {
            Rx::abort("Could not load HostFXR get property function");
        }

        if(hostfxr_set_runtime_property_value == nullptr) {
            Rx::abort("Could not load HostFXR set property function");
        }

        if(hostfxr_create_delegate == nullptr) {
            Rx::abort("Could not load HostFXR create delegate function");
        }
    }

    void Host::add_managed_assembly_to_tpa_list() const {
        const char_t* tpa_list;
        auto result = hostfxr_get_runtime_property_value(host_context, TPA_PROPERTY, &tpa_list);
        if(result != 0) {
            Rx::abort("Could not get TPA list");
        }
        auto tpa_list_string = Rx::WideString{reinterpret_cast<const Uint16*>(tpa_list)}.to_utf8();
        tpa_list_string += ";E:\\Documents\\SanityEngine\\build\\SanityEngine\\Debug\\SanityEngine.NET.dll";
        const auto tpa_list_wide_string = tpa_list_string.to_utf16();
        result = hostfxr_set_runtime_property_value(host_context,
                                                    TPA_PROPERTY,
                                                    reinterpret_cast<const char_t*>(tpa_list_wide_string.data()));
        if(result != 0) {
            Rx::abort("Could not add managed assembly to TPA list");
        }
    }
} // namespace coreclr
