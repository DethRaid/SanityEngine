#pragma once

#include "core/async/synchronized_resource.hpp"
#include "entity_scripting_api.hpp"
#include "entt/entity/fwd.hpp"
#include "rx/core/map.h"
#include "rx/core/optional.h"
#include "rx/core/ptr.h"
#include "rx/core/string.h"
#include "wren.hpp"

class World;

namespace std {
    namespace filesystem {
        class path;
    }
} // namespace std

namespace script {
    struct ScriptingClassName {
        Rx::String module_name{};

        Rx::String class_name{};
    };

    struct ScriptingFunctionName {
        Rx::String module_name{};

        Rx::String class_name{};

        bool is_static{false};

        Rx::String method_signature{};
    };

    struct UiPanelInstance {
        WrenHandle* draw_method_handle;

        WrenHandle* instance_handle;
    };

    class ScriptingRuntime {
    public:
        static Rx::Ptr<ScriptingRuntime> create(SynchronizedResource<entt::registry>& registry_in);

        explicit ScriptingRuntime(WrenVM* vm_in, SynchronizedResource<entt::registry>& registry_in);

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

        void register_script_object_allocator(const ScriptingClassName& name, const WrenForeignClassMethods& allocator_methods);

        void register_script_function(const ScriptingFunctionName& name, WrenForeignMethodFn function);

        [[nodiscard]] WrenVM* get_vm() const;

        [[nodiscard]] WrenHandle* create_entity() const;

        [[nodiscard]] WrenHandle* instantiate_script_object(const Rx::String& module_name, const Rx::String& class_name) const;

        [[nodiscard]] Rx::Optional<Component> create_component(entt::entity entity,
                                                               const char* module_name,
                                                               const char* component_class_name) const;

        // The codegen will place generated methods in this region. Everything within this region will be destroyed whenever the scripting
        // API is regenerated - aka very often. DO NOT put any code you care about saving in this region
        // TODO: Be sure to generate comments telling Resharper to take a heckin' chill pill
#pragma region Wren API
        // ReSharper disable once CppInconsistentNaming
        static void _entity_get_tags(WrenVM* vm);
#pragma endregion

    private:
        using WrenClass = Rx::Map<Rx::String, WrenForeignMethodFn>;

        struct WrenModule {
            Rx::Map<Rx::String, WrenClass> classes;
            Rx::Map<Rx::String, WrenForeignClassMethods> object_allocators;
        };

        using WrenProgram = Rx::Map<Rx::String, WrenModule>;

        // One WrenProgram for static methods, one for non-static
        Rx::Map<bool, WrenProgram> registered_script_functions{};

        static void wren_error(WrenVM* vm, WrenErrorType type, const char* module_name, int line, const char* message);

        static void wren_log(WrenVM* vm, const char* text);

        [[nodiscard]] static WrenForeignMethodFn wren_bind_foreign_method(
            WrenVM* vm, const char* module_name, const char* class_name, bool is_static, const char* signature);

        [[nodiscard]] static WrenForeignClassMethods wren_bind_foreign_class(WrenVM* vm, const char* module_name, const char* class_name);

        [[nodiscard]] static char* wren_load_module(WrenVM* vm, const char* module_name);

        WrenVM* vm{nullptr};

        SynchronizedResource<entt::registry>* registry;

        Rx::Set<std::filesystem::path> module_paths{};

        Uint32 load_all_scripts_in_directory(const std::filesystem::path& directory) const;

        [[nodiscard]] WrenForeignMethodFn bind_foreign_method(const Rx::String& module_name,
                                                              const Rx::String& class_name,
                                                              bool is_static,
                                                              const Rx::String& signature) const;

        [[nodiscard]] WrenForeignClassMethods bind_foreign_class(const Rx::String& module_name, const Rx::String& class_name) const;

        [[nodiscard]] char* load_module(const Rx::String& module_name) const;
    };
} // namespace script
