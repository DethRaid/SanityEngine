#include "scripting_runtime.hpp"

#include <filesystem>
#include <utility>

#include <Windows.h>
#include <hostfxr.h>
#include <nethost.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "../core/ensure.hpp"

std::shared_ptr<spdlog::logger> ScriptingRuntime::logger{spdlog::stdout_color_st("ScriptingRuntime")};

hostfxr_initialize_for_runtime_config_fn init_fptr;
hostfxr_get_runtime_delegate_fn get_delegate_fptr;
hostfxr_close_fn close_fptr;

load_assembly_and_get_function_pointer_fn get_dotnet_assembly_fptr;

[[nodiuscard]] bool load_hostfxr() {
    char_t buffer[MAX_PATH];
    auto buffer_size = sizeof(buffer) / sizeof(char_t);
    const auto rc = get_hostfxr_path(buffer, &buffer_size, nullptr);
    if(rc != 0) {
        return false;
    }

    // Load hostfxr and get desired exports

    auto* lib = LoadLibraryW(buffer);
    init_fptr = reinterpret_cast<hostfxr_initialize_for_runtime_config_fn>(GetProcAddress(lib, "hostfxr_initialize_for_runtime_config"));
    get_delegate_fptr = reinterpret_cast<hostfxr_get_runtime_delegate_fn>(GetProcAddress(lib, "hostfxr_get_runtime_delegate"));
    close_fptr = reinterpret_cast<hostfxr_close_fn>(GetProcAddress(lib, "hostfxr_close"));

    return (init_fptr && get_delegate_fptr && close_fptr);
}

// Most of the code here comes from Microsoft's custom app host samples

std::optional<ScriptingRuntime> ScriptingRuntime::create(const std::string& assembly_path, const std::string& assembly_name) {
    if(!load_hostfxr()) {
        logger->error("Could not load HostFXR");
    }

    const auto config_path = std::filesystem::path{assembly_path} / assembly_path / ".json";
    if(exists(config_path)) {
        const load_assembly_and_get_function_pointer_fn load_assembly_and_get_function_pointer_local = get_dotnet_load_assembly(
            config_path);
        ENSURE(load_assembly_and_get_function_pointer_local != nullptr, "Failure: get_dotnet_load_assembly()");

        return std::make_optional<ScriptingRuntime>(load_assembly_and_get_function_pointer_local);
    }

    return std::nullopt;
}

std::optional<ComponentClass> ScriptingRuntime::load_scripting_class(const std::wstring& class_name) const {
    constexpr const char_t* create_method_name = L"Create";
    CreateScriptingComponent create_ptr;
    auto rc = load_assembly_and_get_function_pointer(assembly_path.c_str(),
                                                     class_name.c_str(),
                                                     create_method_name,
                                                     nullptr,
                                                     nullptr,
                                                     reinterpret_cast<void**>(&create_ptr));
    if(rc != 0) {
        logger->error("Could not load method {}::{} from assembly {}: {#x}", class_name, create_method_name, assembly_path, rc);
        return std::nullopt;
    }

    constexpr const char_t* tick_method_name = L"Tick";
    TickScriptingComponent tick_ptr;
    rc = load_assembly_and_get_function_pointer(assembly_path.c_str(),
                                                class_name.c_str(),
                                                tick_method_name,
                                                nullptr,
                                                nullptr,
                                                reinterpret_cast<void**>(&tick_ptr));
    if(rc != 0) {
        logger->error("Could not load method {}::{} from assembly {}: {#x}", class_name, tick_method_name, assembly_path, rc);
        return std::nullopt;
    }

    constexpr const char_t* destroy_method_name = L"Destroy";
    DestroyScriptingComponent destroy_ptr;
    rc = load_assembly_and_get_function_pointer(assembly_path.c_str(),
                                                class_name.c_str(),
                                                destroy_method_name,
                                                nullptr,
                                                nullptr,
                                                reinterpret_cast<void**>(&destroy_ptr));
    if(rc != 0) {
        logger->error("Could not load method {}::{} from assembly {}: {#x}", class_name, destroy_method_name, assembly_path, rc);
        return std::nullopt;
    }

    return std::make_optional<ComponentClass>(create_ptr, tick_ptr, destroy_ptr);
}

load_assembly_and_get_function_pointer_fn ScriptingRuntime::get_dotnet_load_assembly(const std::filesystem::path& config_path) {
    // Load .NET Core
    void* load_assembly_and_get_function_pointer = nullptr;
    hostfxr_handle cxt = nullptr;
    int rc = init_fptr(config_path.c_str(), nullptr, &cxt);
    if(rc != 0 || cxt == nullptr) {
        logger->error("Init failed: {#x}", rc);
        close_fptr(cxt);
        return nullptr;
    }

    // Get the load assembly function pointer
    rc = get_delegate_fptr(cxt, hdt_load_assembly_and_get_function_pointer, &load_assembly_and_get_function_pointer);
    if(rc != 0 || load_assembly_and_get_function_pointer == nullptr) {
        logger->error("Get delegate failed: {#x}", rc);
    }

    close_fptr(cxt);
    return static_cast<load_assembly_and_get_function_pointer_fn>(load_assembly_and_get_function_pointer);
}

ScriptingRuntime::ScriptingRuntime(std::wstring assembly_path_in,
                                   const load_assembly_and_get_function_pointer_fn load_assembly_and_get_function_pointer_in)
    : assembly_path{std::move(assembly_path_in)}, load_assembly_and_get_function_pointer{load_assembly_and_get_function_pointer_in} {}
