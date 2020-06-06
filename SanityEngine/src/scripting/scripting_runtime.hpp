#pragma once

#include <spdlog/logger.h>
#include <wrl/client.h>

#import "scriptingapi.tlb" raw_interfaces_only

using Microsoft::WRL::ComPtr;

struct ScriptingComponent {
    ScriptingApi::_GameplayComponentPtr ptr;
};

class ScriptingRuntime {
public:
    ScriptingRuntime();

    ScriptingRuntime(const ScriptingRuntime& other) = delete;
    ScriptingRuntime& operator=(const ScriptingRuntime& other) = delete;

    ScriptingRuntime(ScriptingRuntime&& old) noexcept = default;
    ScriptingRuntime& operator=(ScriptingRuntime&& old) noexcept = default;

    ~ScriptingRuntime();

    void load_library(std::wstring& library_path);

    void unload_library(const std::wstring& library_path);

    ScriptingApi::_GameplayComponentPtr create_component(CLSID class_guid);

private:
    static std::shared_ptr<spdlog::logger> logger;

    std::unordered_map<std::wstring, HINSTANCE> loaded_libraries;

    // std::unordered_map<CLSID, ComPtr<IClassFactory>> class_factories;
};
