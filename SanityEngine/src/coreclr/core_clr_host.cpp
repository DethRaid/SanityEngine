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

        const char* property_keys[] = {"TRUSTED_PLATFORM_ASSEMBLIES"};

        const char* property_values[] = {tpa_list.data()};

        const auto result = coreclr_initialize_func(coreclr_working_directory.data(),
                                                    "SanityEngine",
                                                    sizeof(property_keys) / sizeof(char*),
                                                    property_keys,
                                                    property_values,
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
        // TODO
    }
} // namespace coreclr
