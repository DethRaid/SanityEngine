#pragma once

#include <optional>

#include <spdlog/logger.h>
#include <wren/wren.hpp>

struct ScriptComponentMethods {
    WrenHandle* init_handle;

    WrenHandle* begin_play_handle;

    WrenHandle* tick_handle;

    WrenHandle* end_play_handle;
};

struct ScriptingComponent {
    ScriptComponentMethods class_methods;

    WrenHandle* component_handle;
};

class ScriptingRuntime {
public:
    static std::optional<ScriptingRuntime> create();

    explicit ScriptingRuntime(WrenVM* vm_in, const ScriptComponentMethods& methods_in);

    ScriptingRuntime(const ScriptingRuntime& other) = delete;
    ScriptingRuntime& operator=(const ScriptingRuntime& other) = delete;

    ScriptingRuntime(ScriptingRuntime&& old) noexcept;
    ScriptingRuntime& operator=(ScriptingRuntime&& old) noexcept;

    ~ScriptingRuntime();

    [[nodiscard]] std::optional<ScriptingComponent> create_component(const std::string& component_class_name);

private:
    static std::shared_ptr<spdlog::logger> logger;
    static std::shared_ptr<spdlog::logger> script_logger;

    static void wren_error(WrenVM* vm, WrenErrorType type, const char* module_name, int line, const char* message);

    static void wren_log(WrenVM* vm, const char* text);

    static WrenForeignMethodFn wren_resolve_foreign_method(
        WrenVM* vm, const char* module_name, const char* class_name, bool is_static, const char* signature);

    WrenVM* vm{nullptr};

    ScriptComponentMethods methods;

    WrenForeignMethodFn resolve_foreign_method(const std::string& module_name,
                                               const std::string& class_name,
                                               bool is_static,
                                               const std::string& signature);
};
