#include "scripting_runtime.hpp"

#include <minitrace.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "../core/errors.hpp"
#include "../rhi/helpers.hpp"

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

void ScriptingRuntime::load_library(std::wstring& library_path) {
    const auto library_handle = LoadLibrary(library_path.c_str());
    loaded_libraries.emplace(library_path, library_handle);
}

void ScriptingRuntime::unload_library(const std::wstring& library_path) {
    if(const auto lib_itr = loaded_libraries.find(library_path); lib_itr != loaded_libraries.end()) {
        FreeLibrary(lib_itr->second);
        loaded_libraries.erase(library_path);
    }
}

ScriptingApi::_GameplayComponentPtr ScriptingRuntime::create_component(const CLSID class_guid) {
    MTR_SCOPE("ScriptingRuntime", "create_component");

    ScriptingApi::_GameplayComponentPtr component;

   /* if(const auto class_factory_itr = class_factories.find(class_guid); class_factory_itr != class_factories.end()) {
        class_factory_itr->second->CreateInstance(nullptr, IID_PPV_ARGS(&component));

    } else {*/
        for(const auto& [name, library] : loaded_libraries) {
            const auto dll_get_class_object = reinterpret_cast<LPFNGETCLASSOBJECT>(GetProcAddress(library, "DllGetClassObject"));
            if(dll_get_class_object == nullptr) {
                logger->error("Could not load DllGetClassObject method from library {}", rhi::from_wide_string(name));
                continue;
            }

            ComPtr<IClassFactory> class_factory;
            const auto result = dll_get_class_object(class_guid, IID_PPV_ARGS(&class_factory));
            if(FAILED(result)) {
                // That library didn't have a factory for our type, try the next one
                continue;
            }

            // class_factories.emplace(class_guid, class_factory);

            class_factory->CreateInstance(nullptr, IID_PPV_ARGS(&component));
        }
    //}

    if(component == nullptr) {
        logger->error("Could not create component");
    }

    return component;
}
