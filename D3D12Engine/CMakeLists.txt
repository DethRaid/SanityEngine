# CMakeList.txt : CMake project for D3D12Engine, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Add source to this project's executable.

set(D3D12_ENGINE_SOURCE
    "${CMAKE_CURRENT_LIST_DIR}/extern/minitrace/minitrace.c"  

    "src/D3D12Engine.cpp" 
    "src/D3D12Engine.hpp" 

    "src/core/constants.hpp"

    "src/logging/stdoutstream.cpp" 
    "src/logging/stdoutstream.hpp" 

    "src/render/renderer.hpp"
    "src/render/command_list.hpp"
    "src/render/resource_command_list.hpp"
    "src/render/compute_command_list.hpp"
    "src/render/graphics_command_list.hpp"
)

set(D3D12_BACKEND_SOURCE
    "src/render/d3d12/d3d12_render_device.hpp"
    "src/render/d3d12/d3d12_render_device.cpp"

    "src/render/d3d12/helpers.hpp"
    "src/render/d3d12/helpers.cpp"
    )

set(D3D12_ENGINE_SOURCE 
    ${D3D12_ENGINE_SOURCE}
    ${D3D12_BACKEND_SOURCE})

add_executable(D3D12Engine ${D3D12_ENGINE_SOURCE})

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/extern/rex_core)

target_link_libraries(D3D12Engine rex)

target_include_directories(D3D12Engine PRIVATE ${CMAKE_CURRENT_LIST_DIR}/extern/minitrace)

target_compile_options(D3D12Engine PRIVATE -Wno-deprecated-declarations)
target_compile_definitions(D3D12Engine PRIVATE NOMINMAX)
