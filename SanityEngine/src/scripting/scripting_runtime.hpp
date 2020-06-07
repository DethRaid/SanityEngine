#pragma once

#include <optional>
#include <set>
#include <unordered_set>

#include <entt/entity/fwd.hpp>
#include <spdlog/logger.h>
#include <wren/wren.hpp>

class World;

namespace std {
    namespace filesystem {
        class path;
    }
} // namespace std

struct ScriptComponentMethods {
    WrenHandle* init_handle;

    WrenHandle* begin_play_handle;

    WrenHandle* tick_handle;

    WrenHandle* end_play_handle;
};

class ScriptingComponent {
public:
    explicit ScriptingComponent(WrenHandle* handle_in, const ScriptComponentMethods& class_methods_in, WrenVM& vm_in);

    ScriptingComponent(const ScriptingComponent& other) = default;
    ScriptingComponent& operator=(const ScriptingComponent& other) = default;

    ScriptingComponent(ScriptingComponent&& old) noexcept = default;
    ScriptingComponent& operator=(ScriptingComponent&& old) noexcept = default;

    ~ScriptingComponent() = default;

    void initialize_self();

    void begin_play(World& world) const;

    void tick(float delta_seconds) const;

    void end_play() const;

private:
    ScriptComponentMethods class_methods;

    WrenHandle* component_handle;

    WrenVM* vm;
};

struct ScriptingClassName {
    std::string module_name{};

    std::string class_name{};
};

struct ScriptingFunctionName {
    std::string module_name{};

    std::string class_name{};

    bool is_static{false};

    std::string method_signature{};
};

class ScriptingRuntime {
public:
    static std::unique_ptr<ScriptingRuntime> create(entt::registry& registry_in);

    explicit ScriptingRuntime(WrenVM* vm_in, entt::registry& registry_in);

    ScriptingRuntime(const ScriptingRuntime& other) = delete;
    ScriptingRuntime& operator=(const ScriptingRuntime& other) = delete;

    ScriptingRuntime(ScriptingRuntime&& old) noexcept;
    ScriptingRuntime& operator=(ScriptingRuntime&& old) noexcept;

    ~ScriptingRuntime();

    /*!
     * \brief Loads all the scripts in the specified directory into the Wren VM
     */
    [[nodiscard]] bool add_script_directory(const std::filesystem::path& directory);

    void remove_script_directory(const std::filesystem::path& directory);

    void register_script_object_allocator(const ScriptingClassName& name, WrenForeignClassMethods allocator_methods);

    void register_script_function(const ScriptingFunctionName& name, WrenForeignMethodFn function);

    [[nodiscard]] std::optional<ScriptingComponent> create_component(const std::string& module_name,
                                                                     const std::string& component_class_name) const;

    // The codegen will place generated methods in this region. Everything within this region will be destroyed whenever the scripting API
    // is regenerated - aka very often. DO NOT put any code you care about saving in this region
    // TODO: Be sure to generate comments telling Resharper to take a heckin' chill pill
#pragma region Wren API
    // ReSharper disable once CppInconsistentNaming
    static void _entity_get_tags(WrenVM* vm);
#pragma endregion

private:
    static std::shared_ptr<spdlog::logger> logger;
    static std::shared_ptr<spdlog::logger> script_logger;

    using WrenClass = std::unordered_map<std::string, WrenForeignMethodFn>;

    struct WrenModule {
        std::unordered_map<std::string, WrenClass> classes;
        std::unordered_map<std::string, WrenForeignClassMethods> object_allocators;
    };

    using WrenProgram = std::unordered_map<std::string, WrenModule>;

    // One WrenProgram for static methods, one for non-static
    std::unordered_map<bool, WrenProgram> registered_script_functions{};

    static void wren_error(WrenVM* vm, WrenErrorType type, const char* module_name, int line, const char* message);

    static void wren_log(WrenVM* vm, const char* text);

    [[nodiscard]] static WrenForeignMethodFn wren_bind_foreign_method(
        WrenVM* vm, const char* module_name, const char* class_name, bool is_static, const char* signature);

    [[nodiscard]] static WrenForeignClassMethods wren_bind_foreign_class(WrenVM* vm, const char* module_name, const char* class_name);

    [[nodiscard]] static const char* wren_resolve_module(WrenVM* vm, const char* importer, const char* name);

    [[nodiscard]] static char* wren_load_module(WrenVM* vm, const char* module_name);

    WrenVM* vm{nullptr};

    entt::registry* registry;

    std::set<std::filesystem::path> module_paths{};

    void load_all_scripts_in_directory(const std::filesystem::path& directory) const;

    [[nodiscard]] WrenForeignMethodFn bind_foreign_method(const std::string& module_name,
                                                          const std::string& class_name,
                                                          bool is_static,
                                                          const std::string& signature) const;

    [[nodiscard]] WrenForeignClassMethods bind_foreign_class(const std::string& module_name, const std::string& class_name) const;

    [[nodiscard]] char* load_module(const std::string& module_name) const;
};
