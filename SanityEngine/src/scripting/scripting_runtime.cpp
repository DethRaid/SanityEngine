#include "scripting_runtime.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>

#include "../core/errors.hpp"

std::shared_ptr<spdlog::logger> ScriptingRuntime::logger{spdlog::stdout_color_st("ScriptingRuntime")};

ScriptingRuntime::ScriptingRuntime() {
    const auto result = CoInitialize(nullptr);
    if(FAILED(result)) {
        logger->error("Could not initialize COM: {} (Error code {#x}", to_string(result), result);
    }
}

ScriptingRuntime::~ScriptingRuntime() { CoUninitialize(); }

ScriptingApi::_GameplayComponentPtr ScriptingRuntime::create_script_component(const CLSID class_guid) {
    ComPtr<IClassFactory> class_factory;
    const auto result = CoGetClassObject(class_guid, CLSCTX_APPCONTAINER, nullptr, IID_PPV_ARGS(&class_factory));
    if(FAILED(result)) {
        // TODO: Figure out how to print class IDs
        logger->error("Can not load class: {} (Error code {#x})", to_string(result), result);
        return {};
    }

    ScriptingApi::_GameplayComponentPtr component;
    class_factory->CreateInstance(nullptr, IID_PPV_ARGS(&component));

    return component;
}
