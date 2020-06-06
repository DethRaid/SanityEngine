#include "scripting_runtime.hpp"


#include <minitrace.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <wrl/client.h>

#include "../core/errors.hpp"

using Microsoft::WRL::ComPtr;

std::shared_ptr<spdlog::logger> ScriptingRuntime::logger{spdlog::stdout_color_st("ScriptingRuntime")};

ScriptingRuntime::ScriptingRuntime() {
    MTR_SCOPE("ScriptingRuntime", "ScriptingRuntime");
    const auto result = CoInitialize(nullptr);
    if(FAILED(result)) {
        logger->error("Could not initialize COM: {} (Error code {x}", to_string(result), result);
        throw std::exception{"Can't initialize COM"};
    }
}

ScriptingRuntime::~ScriptingRuntime() { CoUninitialize(); }

ScriptingApi::_GameplayComponentPtr ScriptingRuntime::create_component(const CLSID class_guid) {
    MTR_SCOPE("ScriptingRuntime", "create_component");

    ComPtr<IClassFactory> class_factory;
    const auto result = CoGetClassObject(class_guid, CLSCTX_INPROC_SERVER, nullptr, IID_PPV_ARGS(&class_factory));
    if(FAILED(result)) {
        // TODO: Figure out how to print class IDs
        logger->error("Can not load class factory: {}", to_string(result));
        return {};
    }

    ScriptingApi::_GameplayComponentPtr component;
    class_factory->CreateInstance(nullptr, IID_PPV_ARGS(&component));

    return component;
}
