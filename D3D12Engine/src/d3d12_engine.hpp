#pragma once
#include <memory>

#include "debugging/renderdoc_app.h"
#include "render/renderer.hpp"
#include "settings.hpp"

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
    Settings settings;

    std::unique_ptr<RENDERDOC_API_1_3_0> renderdoc;

    std::unique_ptr<render::RenderDevice> render_device;

    GLFWwindow* window;

    /*!
     * \brief Ticks the engine, advancing time by the specified amount
     */
    void tick(double delta_time);
};
