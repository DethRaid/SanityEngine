#include "core_clr_host.hpp"

#include <filesystem>

#include "Tracy.hpp"
#include "core/errors.hpp"
#include "core/types.hpp"
#include "rx/core/abort.h"
#include "rx/core/log.h"
#include "rx/core/string.h"

namespace coreclr {
    RX_LOG("\033[35;47mCoreCLR\033[0m", logger);

    Host::Host(const Rx::String& coreclr_working_directory) {
        ZoneScoped;

        const auto core_clr_assembly_path = Rx::String::format("%s/coreclr.dll", coreclr_working_directory);
        coreclr = LoadLibraryExA(core_clr_assembly_path.data(), nullptr, 0);
        if(coreclr == nullptr) {
            Rx::abort("Could not load CoreCLR assembly at '%s'", core_clr_assembly_path);
        }

        logger->verbose("CoreCLR assembly loaded");

        coreclr_initialize_func = reinterpret_cast<coreclr_initialize_ptr>(GetProcAddress(coreclr, "coreclr_initialize"));
        coreclr_create_delegate_func = reinterpret_cast<coreclr_create_delegate_ptr>(GetProcAddress(coreclr, "coreclr_create_delegate"));
        coreclr_shutdown_func = reinterpret_cast<coreclr_shutdown_ptr>(GetProcAddress(coreclr, "coreclr_shutdown"));

        if(coreclr_initialize_func == nullptr) {
            Rx::abort("Could not load CoreCLR initialize function");
        }

        if(coreclr_create_delegate_func == nullptr) {
            Rx::abort("Could not load CoreCLR create delegate function");
        }

        if(coreclr_shutdown_func == nullptr) {
            Rx::abort("Could not load CoreCLR shutdown function");
        }

        Rx::String tpa_list;
        tpa_list.reserve(coreclr_working_directory.size() * 32); // Wild guess

        // Note that the CLR does not guarantee which assembly will be loaded if an assembly is in the TPA list multiple times (perhaps from
        // different paths or perhaps with different NI/NI.dll extensions. Therefore, a real host should probably add items to the list in
        // priority order and only add a file if it's not already present on the list.
        const auto coreclr_path = std::filesystem::path{coreclr_working_directory.data()};
        for(const auto& child_path : coreclr_path) {
            if(child_path.extension() == ".dll") {
                const auto& child_path_string = child_path.string();
                tpa_list += child_path_string.c_str();
                tpa_list += ";";
            }
        }

        Rx::Vector<const char*> property_keys = Rx::Array{"TRUSTED_PLATFORM_ASSEMBLIES", "APP_PATHS"};

        const char* tpa_list_ptr = tpa_list.data();
        Rx::Vector<const char*> property_values = Rx::Array{tpa_list_ptr,
                                                            R"(E:\Documents\SanityEngine\SanityEngine-CSharp\bin\Debug\netcoreapp3.1)"};

        const auto result = coreclr_initialize_func(coreclr_working_directory.data(),
                                                    "SanityEngine",
                                                    property_values.size(),
                                                    property_keys.data(),
                                                    property_values.data(),
                                                    &host_handle,
                                                    &domain_id);
        if(FAILED(result)) {
            Rx::abort("Could not initialize CoreCLR: %s", to_string(result));
        }

        logger->info("Initialized CoreCLR host");
        logger->verbose("Domain ID: %u", domain_id);
    }

    Host::~Host() {
        const auto result = coreclr_shutdown_func(host_handle, domain_id);
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

        const HRESULT result = coreclr_create_delegate_func(host_handle,
                                                            domain_id,
                                                            "SanityEngine-CSharp Version=1.0.0",
                                                            "SanityEngine.EnvironmentObjectEditor",
                                                            "Hi",
                                                            reinterpret_cast<void**>(&hi_function));
        if(FAILED(result)) {
            logger->error("Could not get a pointer to the Hi function: %s", to_string(result));
        } else {
            hi_function();
        }
    }
} // namespace coreclr
