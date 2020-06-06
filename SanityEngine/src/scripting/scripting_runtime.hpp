#pragma once

#include <fstream>
#include <optional>
#include <string>

#include <coreclr_delegates.h>
#include <spdlog/logger.h>

typedef int (*CreateScriptingComponent)();
typedef void (*TickScriptingComponent)(void* component, float delta_time);
typedef void (*DestroyScriptingComponent)(void* component);

class ComponentClass {
public:
    /*!
     * \brief Creates a new instance of this component class
     */
    CreateScriptingComponent create;

    /*!
     * \brief Ticks this component
     */
    TickScriptingComponent tick;

    /*!
     * \brief Destroys this component
     */
    DestroyScriptingComponent destroy;
};

class ScriptingRuntime {
public:
    /*!
     * \brief Creates a new ScriptingRuntime instance
     *
     * \param assembly_path The path to the scripting assembly which contains all the script code you want to use
     * \param assembly_name The name of the assembly which contains all your scripting code
     */
    static std::optional<ScriptingRuntime> create(const std::string& assembly_path, const std::string& assembly_name);

    ScriptingRuntime(const ScriptingRuntime& other) = default;
    ScriptingRuntime& operator=(const ScriptingRuntime& other) = default;

    ScriptingRuntime(ScriptingRuntime&& old) noexcept = default;
    ScriptingRuntime& operator=(ScriptingRuntime&& old) noexcept = default;

    [[nodiscard]] std::optional<ComponentClass> load_scripting_class(const std::wstring& class_name) const;

private:
    static std::shared_ptr<spdlog::logger> logger;

    static load_assembly_and_get_function_pointer_fn get_dotnet_load_assembly(const std::filesystem::path& config_path);

    std::wstring assembly_path;

    load_assembly_and_get_function_pointer_fn load_assembly_and_get_function_pointer{};

    explicit ScriptingRuntime(std::wstring assembly_path_in,
                              load_assembly_and_get_function_pointer_fn load_assembly_and_get_function_pointer_in);
};
