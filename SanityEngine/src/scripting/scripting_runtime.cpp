#include "scripting_runtime.hpp"

#include <filesystem>

#include <entt/entity/registry.hpp>
#include <minitrace.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "../core/components.hpp"
#include "../rhi/helpers.hpp"
#include "../world/world.hpp"

std::shared_ptr<spdlog::logger> ScriptingRuntime::logger{spdlog::stdout_color_st("ScriptingRuntime")};
std::shared_ptr<spdlog::logger> ScriptingRuntime::script_logger{spdlog::stdout_color_mt("Wren")};

static const std::string SANITY_ENGINE_MODULE_NAME = "SanityEngine";

void ScriptingRuntime::wren_error(
    WrenVM* /* vm */, WrenErrorType /* type */, const char* module_name, const int line, const char* message) {
    script_logger->error("[{}] Line {}: {}", module_name, line, message);
}

void ScriptingRuntime::wren_log(WrenVM* /* vm */, const char* text) { script_logger->info("{}", text); }

WrenForeignMethodFn ScriptingRuntime::wren_bind_foreign_method(
    WrenVM* vm, const char* module_name, const char* class_name, const bool is_static, const char* signature) {
    auto* user_data = wrenGetUserData(vm);
    if(user_data != nullptr) {
        auto* runtime = static_cast<ScriptingRuntime*>(user_data);
        return runtime->bind_foreign_method(module_name, class_name, is_static, signature);
    }

    return nullptr;
}

WrenForeignClassMethods ScriptingRuntime::wren_bind_foreign_class(WrenVM* vm, const char* module_name, const char* class_name) {
    auto* user_data = wrenGetUserData(vm);
    if(user_data != nullptr) {
        auto* runtime = static_cast<ScriptingRuntime*>(user_data);
        return runtime->bind_foreign_class(module_name, class_name);
    }

    return {};
}

const char* ScriptingRuntime::wren_resolve_module(WrenVM* /* vm */, const char* importer, const char* name) {
    const auto resolved_path = std::filesystem::path{importer} / name;

    const auto& resolved_path_string = resolved_path.string();
    auto* resolved_module = new char[resolved_path_string.size()];
    memcpy(resolved_module, resolved_path_string.data(), resolved_path_string.size());

    return resolved_module;
}

char* ScriptingRuntime::wren_load_module(WrenVM* vm, const char* module_name) {
    auto* user_data = wrenGetUserData(vm);
    if(user_data != nullptr) {
        auto* runtime = static_cast<ScriptingRuntime*>(user_data);
        return runtime->load_module(module_name);
    }

    return nullptr;
}

void ScriptingRuntime::load_all_scripts_in_directory(const std::filesystem::path& directory) const {
    if(!exists(directory)) {
        logger->error("Could not load scripts in directory {}: directory does not exist");
        return;
    }
    if(!is_directory(directory)) {
        logger->error("Could not load scripts in directory {}: This path does not refer to a directory");
        return;
    }
    uint32_t num_loaded_modules{0};
    for(const auto& module_entry : std::filesystem::directory_iterator{directory}) {
        const auto& module_path = module_entry.path();
        if(!module_entry.is_directory() && module_path.extension() == ".wren") {
            auto* file = fopen(module_path.string().c_str(), "r");
            if(file == nullptr) {
                // The module wasn't found at this module path - that's perfectly fine! We'll just check another one
                logger->error("Could not open file {}", module_entry);
                fclose(file);
                continue;
            }

            auto result = static_cast<size_t>(fseek(file, 0, SEEK_END));
            if(result != 0) {
                logger->error("Could not get length of file {}", module_entry);
                fclose(file);
                continue;
            }

            const auto length = ftell(file);
            if(length == 0) {
                logger->error("File {} exists, but it has a length of 0", module_entry);
                fclose(file);
                continue;
            }

            rewind(file);

            auto* const module_contents = new char[length + 1];

            result = fread(module_contents, 1, length, file);
            if(result == 0) {
                logger->error("Could not read contents of file {}", module_entry);

                delete[] module_contents;

                fclose(file);
                continue;
            }
            module_contents[result] = 0;

            fclose(file);

            const auto& module_file_name = module_path.filename();

            wrenInterpret(vm, module_file_name.string().c_str(), module_contents);
            logger->info("Successfully loaded script {}", module_entry);
            num_loaded_modules++;

            delete[] module_contents;
        }
    }
    if(num_loaded_modules > 0) {
        logger->warn(
            "No modules loaded from directory {}. If you are planning on adding scripts here while the application is running, you may ignore this warning",
            directory);
    }
}

WrenForeignMethodFn ScriptingRuntime::bind_foreign_method(const std::string& module_name,
                                                          const std::string& class_name,
                                                          const bool is_static,
                                                          const std::string& signature) const {
    if(const auto& program_itr = registered_script_functions.find(is_static); program_itr != registered_script_functions.end()) {
        if(const auto& module_itr = program_itr->second.find(module_name); module_itr != program_itr->second.end()) {
            if(const auto& class_itr = module_itr->second.classes.find(class_name); class_itr != module_itr->second.classes.end()) {
                if(const auto& method_itr = class_itr->second.find(signature); method_itr != class_itr->second.end()) {
                    return method_itr->second;
                }
            }
        }
    }

    return nullptr;
}

WrenForeignClassMethods ScriptingRuntime::bind_foreign_class(const std::string& module_name, const std::string& class_name) const {
    if(const auto& program_itr = registered_script_functions.find(true); program_itr != registered_script_functions.end()) {
        if(const auto& module_itr = program_itr->second.find(module_name); module_itr != program_itr->second.end()) {
            if(const auto& class_allocator_itr = module_itr->second.object_allocators.find(class_name);
               class_allocator_itr != module_itr->second.object_allocators.end()) {
                return class_allocator_itr->second;
            }
        }
    }

    return {};
}

char* ScriptingRuntime::load_module(const std::string& module_name) const { // Check all of the registered script paths
    if(module_paths.empty()) {
        logger->error("No registered script module paths! You must register a script module path if you want to load scripts");
        return nullptr;
    }

    for(const auto& module_directory : module_paths) {
        const auto& potential_filename = module_directory / module_name;
        logger->trace("Attempting to load file {} for module {}", potential_filename, module_name);

        auto* file = fopen(module_name.c_str(), "r");
        if(file == nullptr) {
            // The module wasn't found at this module path - that's perfectly fine! We'll just check another one
            logger->trace("Could not open file {}", potential_filename);
            fclose(file);
            return nullptr;
        }

        auto result = static_cast<size_t>(fseek(file, 0, SEEK_END));
        if(result != 0) {
            logger->warn("Could not get length of file {}", potential_filename);
            fclose(file);
            continue;
        }

        const auto length = ftell(file);
        if(length == 0) {
            logger->warn("File {} exists, but it has a length of 0", potential_filename);
            fclose(file);
            continue;
        }

        result = fseek(file, 0, SEEK_SET);
        if(result != 0) {
            logger->warn("Could not reset file pointer for file {}", potential_filename);
            fclose(file);
            continue;
        }

        auto* const module_contents = new char[length];

        result = fread(module_contents, length * sizeof(char), 1, file);
        if(result == 0) {
            logger->warn("Could not read contents of file {}", potential_filename);

            delete[] module_contents;

            fclose(file);
            continue;
        }

        fclose(file);

        return module_contents;
    }

    return nullptr;
}

ScriptingComponent::ScriptingComponent(WrenHandle* handle_in, const ScriptComponentMethods& class_methods_in, WrenVM& vm_in)
    : class_methods{class_methods_in}, component_handle{handle_in}, vm{&vm_in} {}

void ScriptingComponent::initialize_self() {}

void ScriptingComponent::begin_play(World& world) const {
    wrenEnsureSlots(vm, 2);
    wrenSetSlotHandle(vm, 0, component_handle);
    wrenSetSlotHandle(vm, 1, world._get_wren_handle());

    wrenCall(vm, class_methods.begin_play_handle);
}

void ScriptingComponent::tick(const float delta_seconds) const {
    wrenEnsureSlots(vm, 2);
    wrenSetSlotHandle(vm, 0, component_handle);
    wrenSetSlotDouble(vm, 1, delta_seconds);

    wrenCall(vm, class_methods.tick_handle);
}

void ScriptingComponent::end_play() const {
    wrenEnsureSlots(vm, 1);
    wrenSetSlotHandle(vm, 0, component_handle);

    wrenCall(vm, class_methods.end_play_handle);
}

std::unique_ptr<ScriptingRuntime> ScriptingRuntime::create(entt::registry& registry_in) {
    MTR_SCOPE("ScriptingRuntime", "create");

    auto config = WrenConfiguration{};
    wrenInitConfiguration(&config);

    config.errorFn = wren_error;
    config.writeFn = wren_log;
    config.bindForeignMethodFn = wren_bind_foreign_method;
    config.bindForeignClassFn = wren_bind_foreign_class;
    config.resolveModuleFn = wren_resolve_module;
    config.loadModuleFn = wren_load_module;

    auto* vm = wrenNewVM(&config);
    if(vm == nullptr) {
        logger->error("Could not initialize Wren");
        return {};
    }

    return std::make_unique<ScriptingRuntime>(vm, registry_in);
}

ScriptingRuntime::ScriptingRuntime(WrenVM* vm_in, entt::registry& registry_in) : vm{vm_in}, registry{&registry_in} {
    wrenSetUserData(vm, this);
}

ScriptingRuntime::ScriptingRuntime(ScriptingRuntime&& old) noexcept
    : vm{old.vm}, registry{old.registry}, module_paths{std::move(old.module_paths)} {
    old.vm = nullptr;

    wrenSetUserData(vm, this);
}

ScriptingRuntime& ScriptingRuntime::operator=(ScriptingRuntime&& old) noexcept {
    vm = old.vm;
    registry = old.registry;
    module_paths = std::move(old.module_paths);

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
    load_all_scripts_in_directory(directory);

    return true;
}

void ScriptingRuntime::register_script_object_allocator(const ScriptingClassName& name, WrenForeignClassMethods allocator_methods) {
    auto& wren_program = registered_script_functions[true]; // Allocators are always static
    auto& allocators = wren_program[name.module_name].object_allocators;
    allocators.emplace(name.class_name, allocator_methods);
}

void ScriptingRuntime::register_script_function(const ScriptingFunctionName& name, WrenForeignMethodFn function) {
    auto& wren_program = registered_script_functions[name.is_static];
    auto& wren_module = wren_program[name.module_name].classes;
    auto& wren_class = wren_module[name.class_name];

    wren_class.emplace(name.method_signature, function);
}

std::optional<ScriptingComponent> ScriptingRuntime::create_component(const std::string& module_name,
                                                                     const std::string& component_class_name) const {

    // Load the class into a slot so we can get methods from it
    wrenEnsureSlots(vm, 1);
    wrenGetVariable(vm, module_name.c_str(), component_class_name.c_str(), 0);

    const auto full_signature = fmt::format("new()", module_name, component_class_name);
    auto* constructor_handle = wrenMakeCallHandle(vm, "new()");
    if(constructor_handle == nullptr) {
        logger->error("Could not get handle to constructor for {}", component_class_name);
        return std::nullopt;
    }

    const auto result = wrenCall(vm, constructor_handle);
    switch(result) {
        case WREN_RESULT_SUCCESS: {
            auto* component_handle = wrenGetSlotHandle(vm, 0);
            if(component_handle == nullptr) {
                logger->error("Could not create instance of class {}", component_class_name);
            }

            const auto init_signature = fmt::format("{}::init_self()", component_class_name);
            const auto begin_play_signature = fmt::format("{}::begin_play(_)", component_class_name);
            const auto tick_signature = fmt::format("{}::tick(_)", component_class_name);
            const auto end_play_signature = fmt::format("{}::end_play()", component_class_name);

            const auto methods = ScriptComponentMethods{.init_handle = wrenMakeCallHandle(vm, "init_self()"),
                                                        .begin_play_handle = wrenMakeCallHandle(vm, "begin_play(_)"),
                                                        .tick_handle = wrenMakeCallHandle(vm, "tick(_)"),
                                                        .end_play_handle = wrenMakeCallHandle(vm, "end_play()")};

            auto loaded_component_class = true;
            if(methods.init_handle == nullptr) {
                loaded_component_class = false;
                logger->error("Could not load method {}::init");
            }
            if(methods.begin_play_handle == nullptr) {
                loaded_component_class = false;
                logger->error("Could not load component {}::begin_play(_)");
            }
            if(methods.tick_handle == nullptr) {
                loaded_component_class = false;
                logger->error("Could not load component {}::tick(_)");
            }
            if(methods.end_play_handle == nullptr) {
                loaded_component_class = false;
                logger->error("Could not load end play handle");
            }
            if(!loaded_component_class) {
                return std::nullopt;
            }

            return ScriptingComponent{component_handle, methods, *vm};
        }

        case WREN_RESULT_COMPILE_ERROR: {
            logger->error("Compilation error when creating an instance of {}", component_class_name);
            return std::nullopt;
        }

        case WREN_RESULT_RUNTIME_ERROR: {
            logger->error("Runtime error when creating an instance of {}", component_class_name);
            return std::nullopt;
        }

        default: {
            logger->error("Unknown error when creating an instance of {}", component_class_name);
            return std::nullopt;
        }
    }
}

// ReSharper disable once CppInconsistentNaming
void ScriptingRuntime::_entity_get_tags(WrenVM* vm) {
    auto* runtime = static_cast<ScriptingRuntime*>(wrenGetUserData(vm));

    const auto* entity_id_data = wrenGetSlotForeign(vm, 0);
    const auto entity_id = *static_cast<const entt::entity*>(entity_id_data);

    const auto& tag_component = runtime->registry->get<TagComponent>(entity_id);

    wrenEnsureSlots(vm, 2);

    wrenSetSlotNewList(vm, 0);

    int list_idx = 0;
    for(const auto& tag : tag_component.tags) {
        wrenSetSlotString(vm, 1, tag.c_str());

        wrenInsertInList(vm, 0, list_idx, 1);
        list_idx++;
    }
}
