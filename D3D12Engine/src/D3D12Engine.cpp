// D3D12Engine.cpp : Defines the entry point for the application.
//

#include "D3D12Engine.hpp"

#include <minitrace.h>
#include <rx/console/variable.h>
#include <rx/core/global.h>
#include <rx/core/log.h>
#include <stdio.h>
#include <time.h>

#include "logging/stdoutstream.hpp"
#include "core/cvar_names.hpp"

RX_CONSOLE_IVAR(e_num_in_flight_frames, NUM_IN_FLIGHT_FRAMES_NAME, "Maximum number of frames that can be in flight", 1, 5, 3);

static rx::global_group g_nova_globals{"D3D12Engine"};

RX_LOG("D3D12Engine", logger);

static rx::global<StdoutStream> stdout_stream{"system", "stdout_stream"};

int main() {
    D3D12Engine engine;

    engine.run();
}

void D3D12Engine::init_globals() {
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

D3D12Engine::D3D12Engine() {
    MTR_SCOPE("D3D12Engine", "D3D12Engine");

    init_globals();

    logger->info("HELLO HUMAN");
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
