#include "scripting_runtime.hpp"

#include <filesystem>

#include "Tracy.hpp"
#include "adapters/rex/rex_wrapper.hpp"
#include "core/components.hpp"
#include "entt/entity/registry.hpp"
#include "rhi/helpers.hpp"
#include "rx/core/filesystem/file.h"
#include "rx/core/log.h"
#include "ui/wrap_imgui_codegen.h"
#include "world/world.hpp"

namespace script {
    RX_LOG("\033[90;107mScriptingRuntime\033[0m", logger);
    RX_LOG("\033[90;107mWren\033[0m", script_logger);

    static const Rx::String SANITY_ENGINE_MODULE_NAME = "SanityEngine";
    constexpr const char* WREN_CONSTRUCTOR_SIGNATURE = "new()";

    void ScriptingRuntime::wren_error(
        WrenVM* /* vm */, WrenErrorType /* type */, const char* module_name, const int line, const char* message) {
        script_logger->error("[%s] Line %d: %s", module_name, line, message);
    }

    void ScriptingRuntime::wren_log(WrenVM* /* vm */, const char* text) {
        script_logger->info("%s", text);
    }

    WrenForeignMethodFn ScriptingRuntime::wren_bind_foreign_method(
        WrenVM* vm, const char* module_name, const char* class_name, const bool is_static, const char* signature) {
        logger->info("Binding foreign method `%s%s/%s.%s`", is_static ? "static " : "", module_name, class_name, signature);
        if(strcmp(module_name, "imgui") == 0) {
            return wrap_imgui::bindForeignMethod(vm, class_name, is_static, signature);
        }

        auto* user_data = wrenGetUserData(vm);
        if(user_data != nullptr) {
            auto* runtime = static_cast<ScriptingRuntime*>(user_data);
            return runtime->bind_foreign_method(module_name, class_name, is_static, signature);
        }

        return nullptr;
    }

    WrenForeignClassMethods ScriptingRuntime::wren_bind_foreign_class(WrenVM* vm, const char* module_name, const char* class_name) {
        logger->verbose("Binding foreign class `%s/%s`", module_name, class_name);
        if(strcmp(module_name, "imgui") == 0) {
            WrenForeignClassMethods methods = {nullptr, nullptr};
            if(wrap_imgui::bindForeignClass(vm, class_name, methods)) {
                return methods;
            }
        }

        auto* user_data = wrenGetUserData(vm);
        if(user_data != nullptr) {
            auto* runtime = static_cast<ScriptingRuntime*>(user_data);
            return runtime->bind_foreign_class(module_name, class_name);
        }

        return {};
    }

    char* ScriptingRuntime::wren_load_module(WrenVM* vm, const char* module_name) {
        logger->verbose("Loading module %s", module_name);
        Rx::Log::flush();
        if(strcmp(module_name, "imgui") == 0) {
            return wrap_imgui::loadModule(vm);
        }

        auto* user_data = wrenGetUserData(vm);
        if(user_data != nullptr) {
            auto* runtime = static_cast<ScriptingRuntime*>(user_data);
            return runtime->load_module(module_name);
        }

        return nullptr;
    }

    Uint32 ScriptingRuntime::load_all_scripts_in_directory(const std::filesystem::path& directory) const {
        if(!exists(directory)) {
            logger->error("Could not load scripts in directory %s: directory does not exist", directory.string().c_str());
            return 0;
        }
        if(!is_directory(directory)) {
            logger->error("Could not load scripts in directory %s: This path does not refer to a directory", directory.string().c_str());
            return 0;
        }
        Uint32 num_loaded_modules{0};
        for(const auto& module_entry : std::filesystem::directory_iterator{directory}) {
            const auto& module_path = module_entry.path();
            const auto& module_string = module_path.string();

            logger->info("Looking at potential Wren script %s", module_string.c_str());

            if(module_entry.is_directory()) {
                num_loaded_modules += load_all_scripts_in_directory(module_path);

            } else if(module_path.extension() == ".wren") {
                auto* file = fopen(module_path.string().c_str(), "r");
                if(file == nullptr) {
                    // The module wasn't found at this module path - that's perfectly fine! We'll just check another one
                    logger->error("Could not open file %s", module_entry);
                    fclose(file);
                    continue;
                }

                auto result = static_cast<size_t>(fseek(file, 0, SEEK_END));
                if(result != 0) {
                    logger->error("Could not get length of file %s", module_entry);
                    fclose(file);
                    continue;
                }

                const auto length = ftell(file);
                if(length == 0) {
                    logger->error("File %s exists, but it has a length of 0", module_entry);
                    fclose(file);
                    continue;
                }

                rewind(file);

                auto* const module_contents = new char[length + 1];

                result = fread(module_contents, 1, length, file);
                if(result == 0) {
                    logger->error("Could not read contents of file %s", module_entry);

                    delete[] module_contents;

                    fclose(file);
                    continue;
                }
                module_contents[result] = 0;

                fclose(file);

                const auto& module_name_string = module_path.stem().string();

                const auto wren_result = wrenInterpret(vm, module_name_string.c_str(), module_contents);
                switch(wren_result) {
                    case WREN_RESULT_SUCCESS:
                        logger->info("Successfully loaded module %s", module_entry);
                        num_loaded_modules++;
                        break;

                    case WREN_RESULT_COMPILE_ERROR:
                        logger->error("Compile error while loading module %s", module_name_string);
                        break;

                    case WREN_RESULT_RUNTIME_ERROR:
                        logger->error("Runtime error when loading module %s - are you sure you defined all your foreign methods?",
                                      module_name_string);
                        break;

                    default:
                        logger->error("Unknown error when loading module %s");
                }

                delete[] module_contents;
            }
        }
        if(num_loaded_modules == 0) {
            logger->warning(
                "No modules loaded from directory %s. If you are planning on adding scripts here while the application is running, you may ignore this warning",
                directory.string().c_str());
        }

        return num_loaded_modules;
    }

    WrenForeignMethodFn ScriptingRuntime::bind_foreign_method(const Rx::String& module_name,
                                                              const Rx::String& class_name,
                                                              const bool is_static,
                                                              const Rx::String& signature) const {
        if(const auto* wren_program = registered_script_functions.find(is_static)) {
            if(const auto* wren_module = wren_program->find(module_name)) {
                if(const auto* wren_class = wren_module->classes.find(class_name)) {
                    if(const auto* wren_method = wren_class->find(signature)) {
                        return *wren_method;
                    }
                }
            }
        }

        return nullptr;
    }

    WrenForeignClassMethods ScriptingRuntime::bind_foreign_class(const Rx::String& module_name, const Rx::String& class_name) const {
        if(const auto* wren_program = registered_script_functions.find(true)) {
            if(const auto* wren_module = wren_program->find(module_name)) {
                if(const auto* wren_constructor = wren_module->object_allocators.find(class_name)) {
                    return *wren_constructor;
                }
            }
        }

        return {};
    }

    char* ScriptingRuntime::load_module(const Rx::String& module_name) const { // Check all of the registered script paths
        if(module_paths.is_empty()) {
            logger->error("No registered script module paths! You must register a script module path if you want to load scripts");
            return nullptr;
        }

        char* module_contents_output{nullptr};

        module_paths.each([&](const std::filesystem::path& module_directory) {
            const auto& potential_filename = Rx::String::format("%s/%s", module_directory.string().c_str(), module_name);
            logger->verbose("Attempting to load file %s for module %s", potential_filename, module_name);

            auto* file = fopen(module_name.data(), "r");
            if(file == nullptr) {
                // The module wasn't found at this module path - that's perfectly fine! We'll just check another one
                logger->verbose("Could not open file %s", potential_filename);
                return RX_ITERATION_STOP;
            }

            auto result = static_cast<size_t>(fseek(file, 0, SEEK_END));
            if(result != 0) {
                logger->verbose("Could not get length of file %s", potential_filename);
                fclose(file);
                return RX_ITERATION_CONTINUE;
            }

            const auto length = ftell(file);
            if(length == 0) {
                logger->verbose("File %s exists, but it has a length of 0", potential_filename);
                fclose(file);
                return RX_ITERATION_CONTINUE;
            }

            result = fseek(file, 0, SEEK_SET);
            if(result != 0) {
                logger->verbose("Could not reset file pointer for file %s", potential_filename);
                fclose(file);
                return RX_ITERATION_CONTINUE;
            }

            auto* const module_contents = new char[length];

            result = fread(module_contents, length * sizeof(char), 1, file);
            if(result == 0) {
                logger->verbose("Could not read contents of file %s", potential_filename);

                delete[] module_contents;

                fclose(file);
                return RX_ITERATION_CONTINUE;
            }

            fclose(file);

            module_contents_output = module_contents;
            return RX_ITERATION_STOP;
        });

        return module_contents_output;
    }

    Rx::Ptr<ScriptingRuntime> ScriptingRuntime::create(SynchronizedResource<entt::registry>& registry_in) {
        ZoneScoped;

        auto config = WrenConfiguration{};
        wrenInitConfiguration(&config);

        config.errorFn = wren_error;
        config.writeFn = wren_log;
        config.bindForeignMethodFn = wren_bind_foreign_method;
        config.bindForeignClassFn = wren_bind_foreign_class;
        config.loadModuleFn = wren_load_module;

        auto* vm = wrenNewVM(&config);
        if(vm == nullptr) {
            logger->error("Could not initialize Wren");
            return {};
        }

        return Rx::make_ptr<ScriptingRuntime>(RX_SYSTEM_ALLOCATOR, vm, registry_in);
    }

    ScriptingRuntime::ScriptingRuntime(WrenVM* vm_in, SynchronizedResource<entt::registry>& registry_in)
        : vm{vm_in}, registry{&registry_in} {
        wrenSetUserData(vm, this);
    }

    ScriptingRuntime::ScriptingRuntime(ScriptingRuntime&& old) noexcept
        : vm{old.vm}, registry{old.registry}, module_paths{Rx::Utility::move(old.module_paths)} {
        old.vm = nullptr;

        wrenSetUserData(vm, this);
    }

    ScriptingRuntime& ScriptingRuntime::operator=(ScriptingRuntime&& old) noexcept {
        vm = old.vm;
        registry = old.registry;
        module_paths = Rx::Utility::move(old.module_paths);

        old.vm = nullptr;

        wrenSetUserData(vm, this);

        return *this;
    }

    ScriptingRuntime::~ScriptingRuntime() {
        if(vm != nullptr) {
            wrenFreeVM(vm);
        }
    }

    bool ScriptingRuntime::add_script_directory(const std::filesystem::path& directory) {
        module_paths.insert(directory);
        const auto num_scripts_loaded = load_all_scripts_in_directory(directory);

        return num_scripts_loaded > 0;
    }

    void ScriptingRuntime::register_script_object_allocator(const ScriptingClassName& name,
                                                            const WrenForeignClassMethods& allocator_methods) {
        auto& wren_program = *registered_script_functions.find(true); // Allocators are always static
        auto& allocators = wren_program.find(name.module_name)->object_allocators;
        allocators.insert(name.class_name, allocator_methods);
    }

    void ScriptingRuntime::register_script_function(const ScriptingFunctionName& name, WrenForeignMethodFn function) {
        auto& wren_program = *registered_script_functions.find(name.is_static);
        auto& wren_module = wren_program.find(name.module_name)->classes;
        auto& wren_class = *wren_module.find(name.class_name);

        wren_class.insert(name.method_signature, function);
    }

    WrenVM* ScriptingRuntime::get_vm() const { return vm; }

    WrenHandle* ScriptingRuntime::create_entity() const {
        // Load the class into a slot so we can get methods from it
        wrenEnsureSlots(vm, 1);
        wrenGetVariable(vm, "engine", "Entity", 0);

        auto* constructor_handle = wrenMakeCallHandle(vm, WREN_CONSTRUCTOR_SIGNATURE);
        if(constructor_handle == nullptr) {
            logger->error("Could not get handle to constructor for Entity");
            return nullptr;
        }

        const auto result = wrenCall(vm, constructor_handle);
        switch(result) {
            case WREN_RESULT_SUCCESS: {
                auto* component_handle = wrenGetSlotHandle(vm, 0);
                if(component_handle == nullptr) {
                    logger->error("Could not create instance of Entity");
                }

                return component_handle;
            }

            case WREN_RESULT_COMPILE_ERROR: {
                logger->error("Compilation error when creating an Entity");
                return nullptr;
            }

            case WREN_RESULT_RUNTIME_ERROR: {
                logger->error("Runtime error when creating an Entity");
                return nullptr;
            }

            default: {
                logger->error("Unknown error when creating an Entity");
                return nullptr;
            }
        }
    }

    WrenHandle* ScriptingRuntime::instantiate_script_object(const Rx::String& module_name, const Rx::String& class_name) const {
        logger->verbose("Instantiating instance of `%s/%s`", module_name, class_name);

        wrenEnsureSlots(vm, 1);
        wrenGetVariable(vm, module_name.data(), class_name.data(), 0);

        auto* constructor_handle = wrenMakeCallHandle(vm, WREN_CONSTRUCTOR_SIGNATURE);
        if(constructor_handle == nullptr) {
            logger->error("Could not get handle to constructor for %s/%s", module_name, class_name);
            return nullptr;
        }

        const auto result = wrenCall(vm, constructor_handle);
        switch(result) {
            case WREN_RESULT_SUCCESS: {
                auto* component_handle = wrenGetSlotHandle(vm, 0);
                if(component_handle == nullptr) {
                    logger->error("Could not instantiate %s/%s", module_name, class_name);
                    return nullptr;
                }

                return component_handle;
            }

            case WREN_RESULT_COMPILE_ERROR: {
                logger->error("Compilation error when instantiating an %s/%s", module_name, class_name);
                return nullptr;
            }

            case WREN_RESULT_RUNTIME_ERROR: {
                logger->error("Runtime error when instantiating an %s/%s", module_name, class_name);
                return nullptr;
            }

            default: {
                logger->error("Unknown error when instantiating an %s/%s", module_name, class_name);
                return nullptr;
            }
        }

        return nullptr;
    }

    Rx::Optional<Component> ScriptingRuntime::create_component(const entt::entity entity,
                                                               const char* module_name,
                                                               const char* component_class_name) const {

        // Load the class into a slot so we can get methods from it
        wrenEnsureSlots(vm, 1);
        wrenGetVariable(vm, module_name, component_class_name, 0);

        auto* constructor_handle = wrenMakeCallHandle(vm, WREN_CONSTRUCTOR_SIGNATURE);
        if(constructor_handle == nullptr) {
            logger->error("Could not get handle to constructor for %s", component_class_name);
            return Rx::nullopt;
        }

        const auto result = wrenCall(vm, constructor_handle);
        switch(result) {
            case WREN_RESULT_SUCCESS: {
                auto* component_handle = wrenGetSlotHandle(vm, 0);
                if(component_handle == nullptr) {
                    logger->error("Could not create instance of class %s", component_class_name);
                }

                const auto init_signature = Rx::String::format("%s::init_self()", component_class_name);
                const auto begin_play_signature = Rx::String::format("%s::begin_play(_)", component_class_name);
                const auto tick_signature = Rx::String::format("%s::tick(_)", component_class_name);
                const auto end_play_signature = Rx::String::format("%s::end_play()", component_class_name);

                const auto methods = ScriptComponentMethods{.init_handle = wrenMakeCallHandle(vm, "init_self()"),
                                                            .begin_play_handle = wrenMakeCallHandle(vm, "begin_play(_)"),
                                                            .tick_handle = wrenMakeCallHandle(vm, "tick(_)"),
                                                            .end_play_handle = wrenMakeCallHandle(vm, "end_play()")};

                auto loaded_component_class = true;
                if(methods.init_handle == nullptr) {
                    loaded_component_class = false;
                    logger->error("Could not load method %s", init_signature);
                }
                if(methods.begin_play_handle == nullptr) {
                    loaded_component_class = false;
                    logger->error("Could not load method %s", begin_play_signature);
                }
                if(methods.tick_handle == nullptr) {
                    loaded_component_class = false;
                    logger->error("Could not load method %s", tick_signature);
                }
                if(methods.end_play_handle == nullptr) {
                    loaded_component_class = false;
                    logger->error("Could not load method %s", end_play_signature);
                }
                if(!loaded_component_class) {
                    return Rx::nullopt;
                }

                return Component{entity, component_handle, methods, *vm};
            }

            case WREN_RESULT_COMPILE_ERROR: {
                logger->error("Compilation error when creating an instance of %s", component_class_name);
                return Rx::nullopt;
            }

            case WREN_RESULT_RUNTIME_ERROR: {
                logger->error("Runtime error when creating an instance of %s", component_class_name);
                return Rx::nullopt;
            }

            default: {
                logger->error("Unknown error when creating an instance of %s", component_class_name);
                return Rx::nullopt;
            }
        }
    }

    // ReSharper disable once CppInconsistentNaming
    void ScriptingRuntime::_entity_get_tags(WrenVM* vm) {
        auto* runtime = static_cast<ScriptingRuntime*>(wrenGetUserData(vm));

        const auto* entity_id_data = wrenGetSlotForeign(vm, 0);
        const auto entity_id = *static_cast<const entt::entity*>(entity_id_data);

        auto locked_registry = runtime->registry->lock();
        const auto& tag_component = locked_registry->get<SanityEngineEntity>(entity_id);

        wrenEnsureSlots(vm, 2);

        wrenSetSlotNewList(vm, 0);

        int list_idx = 0;
        tag_component.tags.each_key([&](const Rx::String& tag) {
            wrenSetSlotString(vm, 1, tag.data());

            wrenInsertInList(vm, 0, list_idx, 1);
            list_idx++;
        });
    }
} // namespace script
