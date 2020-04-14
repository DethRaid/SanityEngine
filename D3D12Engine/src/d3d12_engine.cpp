// D3D12Engine.cpp : Defines the entry point for the application
//

#include "d3d12_engine.hpp"

#include <minitrace.h>
#include <rx/console/variable.h>
#include <rx/core/global.h>
#include <rx/core/log.h>
#include <rx/core/profiler.h>
#include <stdio.h>
#include <time.h>

#include "core/cvar_names.hpp"
#include "debugging/renderdoc.hpp"
#include "logging/stdoutstream.hpp"

RX_CONSOLE_IVAR(e_num_in_flight_frames, NUM_IN_FLIGHT_FRAMES_NAME, "Maximum number of frames that can be in flight", 1, 5, 3);

RX_CONSOLE_BVAR(r_enable_renderdoc, ENABLE_RENDERDOC_NAME, "Enable the RenderDoc integration for better debugging of graphics code", true);

RX_CONSOLE_BVAR(r_validate_rhi, ENABLE_RHI_VALIDATION_NAME, "Enable runtime validation of the RHI", true);

static rx::global_group g_engine_globals{"D3D12Engine"};

RX_LOG("D3D12Engine", logger);

static rx::global<StdoutStream> stdout_stream{"system", "stdout_stream"};

int main() {
    D3D12Engine engine;

    engine.run();
}

void D3D12Engine::init_globals() const {
    rx::globals::link();

    rx::global_group* system_group{rx::globals::find("system")};

    // Explicitly initialize globals that need to be initialized in a specific
    // order for things to work.
    system_group->find("allocator")->init();
    stdout_stream.init();
    system_group->find("logger")->init();

    const auto subscribed = rx::log::subscribe(&stdout_stream);
    if(!subscribed) {
        fprintf(stderr, "Could not subscribe to logger");
    }

    rx::globals::init();
}

void D3D12Engine::init_rex_profiler() {
    profiler_adapter = rx::make_ptr<RexProfilerAdapter>(*internal_allocator);

    rx::profiler::instance().bind_cpu({profiler_adapter.get(),
                                       +[](void* context, const char* name) {
                                           auto* profiler = static_cast<RexProfilerAdapter*>(context);
                                           profiler->set_thread_name(name);
                                       },
                                       +[](void* context, const char* tag) {
                                           auto* profiler = static_cast<RexProfilerAdapter*>(context);
                                           profiler->begin_sample(tag);
                                       },
                                       +[](void* context) {
                                           auto* profiler = static_cast<RexProfilerAdapter*>(context);
                                           profiler->end_sample();
                                       }});
}

D3D12Engine::D3D12Engine() : internal_allocator{&rx::memory::system_allocator::instance()} {
    MTR_SCOPE("D3D12Engine", "D3D12Engine");

    init_globals();

    init_rex_profiler();

    logger->info("HELLO HUMAN");

    if(*r_enable_renderdoc) {
        renderdoc = load_renderdoc("C:/Users/gold1/bin/RenderDoc/RenderDoc_2020_02_06_fe30fa91_64/renderdoc.dll");
    }

    render_device = make_render_device(*internal_allocator, render::RenderBackend::D3D12);
}

void D3D12Engine::deinit_globals() {
    rx::global_group* system_group{rx::globals::find("system")};

    rx::globals::fini();

    system_group->find("logger")->fini();
    stdout_stream.fini();
    system_group->find("allocator")->fini();
}

D3D12Engine::~D3D12Engine() {
    logger->warning("REMAIN INDOORS");

    deinit_globals();
}

void D3D12Engine::run() {
    double last_frame_duration = 0;

    while(true) {
        const auto start_time = clock();

        tick(last_frame_duration);

        const auto end_time = clock();

        last_frame_duration = static_cast<double>(end_time - start_time) / static_cast<double>(CLOCKS_PER_SEC);
    }
}

void D3D12Engine::tick(double delta_time) { MTR_SCOPE("D3D12Engine", "tick"); }
