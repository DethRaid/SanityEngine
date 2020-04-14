#pragma once
#include <rx/core/ptr.h>

#include "core/rex_profiler_adapter.hpp"
#include "debugging/renderdoc_app.h"
#include "render/renderer.hpp"

/*!
 * \brief Main class for my glorious engine
 */
class D3D12Engine {
public:
    /*!
     * \brief Initializes the engine, including loading static data
     */
    D3D12Engine();

    /*!
     * \brief De-initializes the engine, flushing all logs
     */
    ~D3D12Engine();

    /*!
     * \brief Runs the main loop of the engine. This method eventually returns, after the user is finished playing their game
     */
    void run();

private:
    rx::ptr<RENDERDOC_API_1_3_0> renderdoc;

    rx::ptr<render::RenderDevice> render_device;

    rx::memory::allocator* internal_allocator;

    rx::ptr<RexProfilerAdapter> profiler_adapter;

    void init_globals() const;

    void init_rex_profiler();

    void deinit_globals();

    /*!
     * \brief Ticks the engine, advancing time by the specified amount
     */
    void tick(double delta_time);
};
