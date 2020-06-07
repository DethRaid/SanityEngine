#include "scripting_runtime.hpp"

#include <minitrace.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "../rhi/helpers.hpp"

std::shared_ptr<spdlog::logger> ScriptingRuntime::script_logger{spdlog::stdout_color_mt("Wren")};

static const std::string SANITY_ENGINE_MODULE_NAME = "SanityEngine";

void ScriptingRuntime::wren_error(
    WrenVM* /* vm */, WrenErrorType /* type */, const char* module_name, const int line, const char* message) {
    script_logger->error("[{}] Line {}: {}", module_name, line, message);
}

void ScriptingRuntime::wren_log(WrenVM* /* vm */, const char* text) { script_logger->info("{}", text); }

WrenForeignMethodFn ScriptingRuntime::wren_resolve_foreign_method(
    WrenVM* vm, const char* module_name, const char* class_name, const bool is_static, const char* signature) {
    auto* user_data = wrenGetUserData(vm);
    if(user_data != nullptr) {
        auto* runtime = static_cast<ScriptingRuntime*>(user_data);
        return runtime->resolve_foreign_method(module_name, class_name, is_static, signature);
    }
}

WrenForeignMethodFn ScriptingRuntime::resolve_foreign_method(const std::string& module_name,
                                                             const std::string& class_name,
                                                             const bool is_static,
                                                             const std::string& signature) {
    if(module_name != SANITY_ENGINE_MODULE_NAME) {
        // TODO: forward the result to the game code
        return nullptr;
    }

    if(class_name == "Entity" && signature == "get_tags(_)" && !is_static) {
        
    }

    return nullptr;
}

std::optional<ScriptingRuntime> ScriptingRuntime::create() {
    MTR_SCOPE("ScriptingRuntime", "create");

    auto config = WrenConfiguration{};
    wrenInitConfiguration(&config);

    config.errorFn = wren_error;
    config.writeFn = wren_log;
    config.bindForeignMethodFn = wren_resolve_foreign_method;

    auto* vm = wrenNewVM(&config);
    if(vm == nullptr) {
        logger->error("Could not initialize Wren");
        return std::nullopt;
    }

    const auto methods = ScriptComponentMethods{.init_handle = wrenMakeCallHandle(vm, ""),
                                                .begin_play_handle = wrenMakeCallHandle(vm, ""),
                                                .tick_handle = wrenMakeCallHandle(vm, ""),
                                                .end_play_handle = wrenMakeCallHandle(vm, "")};

    auto loaded_component_class = true;
    if(methods.init_handle == nullptr) {
        loaded_component_class = false;
        logger->error("Could not load component init method");
    }
    if(methods.begin_play_handle == nullptr) {
        loaded_component_class = false;
        logger->error("Could not load component begin play method");
    }
    if(methods.tick_handle == nullptr) {
        loaded_component_class = false;
        logger->error("Could not load component tick method");
    }
    if(methods.end_play_handle == nullptr) {
        loaded_component_class = false;
        logger->error("Could not load end play handle");
    }
    if(!loaded_component_class) {
        return std::nullopt;
    }

    return std::make_optional<ScriptingRuntime>(vm, methods);
}

ScriptingRuntime::ScriptingRuntime(WrenVM* vm_in, const ScriptComponentMethods& methods_in) : vm{vm_in}, methods{methods_in} {
    wrenSetUserData(vm, this);
}

ScriptingRuntime::ScriptingRuntime(ScriptingRuntime&& old) noexcept : vm{old.vm}, methods{old.methods} {
    old.vm = nullptr;

    wrenSetUserData(vm, this);
}

ScriptingRuntime& ScriptingRuntime::operator=(ScriptingRuntime&& old) noexcept {
    vm = old.vm;
    methods = old.methods;

    old.vm = nullptr;

    wrenSetUserData(vm, this);

    return *this;
}

ScriptingRuntime::~ScriptingRuntime() {
    if(vm != nullptr) {
        wrenFreeVM(vm);
    }
}

std::optional<ScriptingComponent> ScriptingRuntime::create_component(const std::string& component_class_name) {
    const auto full_signature = fmt::format("{}::new()", component_class_name);
    auto* constructor_handle = wrenMakeCallHandle(vm, full_signature.c_str());
    if(constructor_handle == nullptr) {
        logger->error("Could not get handle to constructor for {}", component_class_name);
        return std::nullopt;
    }

    wrenEnsureSlots(vm, 1);
    wrenGetVariable(vm, "main", component_class_name.c_str(), 0);

    const auto result = wrenCall(vm, constructor_handle);
    if(result == WREN_RESULT_COMPILE_ERROR) {
        logger->error("Compilation error when creating an instance of {}", component_class_name);
        return std::nullopt;
    } else if(result == WREN_RESULT_RUNTIME_ERROR) {
        logger->error("Runtime error when creating an instance of {}", component_class_name);
        return std::nullopt;
    }

    auto* component_handle = wrenGetSlotHandle(vm, 0);
    if(component_handle == nullptr) {
        logger->error("Could not create instance of class {}", component_class_name);
    }

    return ScriptingComponent{.class_methods = methods, .component_handle = component_handle};
}
