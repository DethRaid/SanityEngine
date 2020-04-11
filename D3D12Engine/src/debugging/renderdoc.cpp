#include "renderdoc.hpp"

#include <libloaderapi.h>
#include <rx/core/log.h>
#include <rx/core/ptr.h>

#include "../windows/windows_helpers.hpp"

RX_LOG("RenderDoc", logger);

rx::ptr<RENDERDOC_API_1_3_0> load_renderdoc(const rx::string& renderdoc_dll_path) {
    HINSTANCE renderdoc_dll = LoadLibrary(renderdoc_dll_path.data());
    if(!renderdoc_dll) {
        const auto error = get_last_windows_error();
        logger->error("Could not load RenderDoc. Error: %s", error);

        return {};
    }

    logger->verbose("Loaded RenderDoc DLL from %s", renderdoc_dll_path);

    const auto get_api = reinterpret_cast<pRENDERDOC_GetAPI>(GetProcAddress(renderdoc_dll, "RENDERDOC_GetAPI"));
    if(!get_api) {
        const auto error = get_last_windows_error();
        logger->error("Could not load RenderDoc DLL. Error: %s", error);

        return {};
    }

    RENDERDOC_API_1_3_0* api;
    const auto ret = get_api(eRENDERDOC_API_Version_1_3_0, reinterpret_cast<void**>(&api));
    if(ret != 1) {
        logger->error("Could not load RenderDoc API");

        return {};
    }

    logger->verbose("Loaded RenderDoc 1.3 API");
    return rx::ptr<RENDERDOC_API_1_3_0>{rx::memory::system_allocator::instance(), api};
}
