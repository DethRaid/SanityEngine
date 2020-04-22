# CMakeList.txt : CMake project for SanityEngine, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Dependencies

set(SPDLOG_MASTER_PROJECT OFF)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/extern/spdlog")

set(GLFW_BUILD_DOCS OFF CACHE BOOL "No docs" FORCE)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/extern/glfw")

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/extern/entt")

# Add source to this project's executable.

set(D3D12_ENGINE_SOURCE
    "${CMAKE_CURRENT_LIST_DIR}/extern/minitrace/minitrace.c"  
    "${CMAKE_CURRENT_LIST_DIR}/extern/D3D12MemoryAllocator/src/D3D12MemAlloc.cpp"

    "src/sanity_engine.cpp"

    "src/core/constants.hpp"
    "src/core/abort.cpp" 
    "src/core/ensure.cpp"

    "src/rhi/render_device.hpp"
    "src/rhi/command_list.hpp"
    "src/rhi/resource_command_list.hpp"
    "src/rhi/compute_command_list.hpp"
    "src/rhi/render_command_list.hpp" 
    "src/rhi/render_device.cpp" 
    "src/rhi/resources.cpp"  
    "src/rhi/mesh_data_store.cpp" 

    "src/debugging/renderdoc.cpp" 
    "src/windows/windows_helpers.cpp" 
 "src/renderer/renderer.cpp")

set(D3D12_BACKEND_SOURCE
    "src/rhi/d3d12/d3d12_render_device.hpp"
    "src/rhi/d3d12/d3d12_render_device.cpp"

    "src/rhi/d3d12/helpers.hpp"
    "src/rhi/d3d12/helpers.cpp"
    "src/rhi/d3d12/d3d12_command_list.cpp"
    "src/rhi/d3d12/d3d12_resource_command_list.cpp"
    "src/rhi/d3d12/d3d12_compute_command_list.cpp"
    "src/rhi/d3d12/d3d12_material.cpp" 
    "src/rhi/d3d12/d3d12_render_command_list.cpp"
    "src/rhi/d3d12/d3d12_descriptor_allocator.cpp" 
    "src/rhi/d3d12/shader_reflection.cpp")

add_executable(SanityEngine ${D3D12_ENGINE_SOURCE} ${D3D12_BACKEND_SOURCE})

target_include_directories(SanityEngine PRIVATE ${CMAKE_CURRENT_LIST_DIR}/extern/minitrace)
target_include_directories(SanityEngine PRIVATE ${CMAKE_CURRENT_LIST_DIR}/extern/D3D12MemoryAllocator/src)

target_compile_options(SanityEngine PRIVATE -Wno-deprecated-declarations -Wno-documentation)
target_compile_definitions(SanityEngine PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)

target_compile_definitions(SanityEngine PRIVATE MTR_ENABLED)

# Hook up dependencies

target_include_directories(SanityEngine PRIVATE "${CMAKE_CURRENT_LIST_DIR}/extern/DirectXMath/Inc")
target_include_directories(SanityEngine PRIVATE "${CMAKE_CURRENT_LIST_DIR}/extern/glfw/include")
target_link_libraries(SanityEngine spdlog glfw d3d12 dxgi d3dcompiler EnTT)

# Compile shaders

find_package(Python3)
if(NOT Python3_Interpreter_FOUND)
    message(ERROR "Could not find Python3 interpreter")
endif()

add_custom_command(TARGET SanityEngine
    PRE_BUILD
    COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_LIST_DIR}/tools/compile_shaders.py" "${CMAKE_CURRENT_LIST_DIR}/data/shaders" "${CMAKE_BINARY_DIR}/data/shaders"
)
