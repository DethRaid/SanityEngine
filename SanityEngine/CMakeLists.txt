# CMakeList.txt : CMake project for SanityEngine, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Global settings

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Don't warn about documentation D3D12_BACKEND_SOURCE, because GLFW doens't use doxygen properly ffs
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-documentation")
endif()

add_compile_definitions(GLM_FORCE_LEFT_HANDED)

# Dependencies

set(SPDLOG_MASTER_PROJECT OFF)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/extern/spdlog")

set(GLFW_BUILD_DOCS OFF CACHE BOOL "No docs" FORCE)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/extern/glfw")

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/extern/entt")

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/extern/glm")

# Add source to this project's executable.

set(SANITY_ENGINE_SOURCE
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

    "src/windows/windows_helpers.cpp" 
    "src/renderer/renderer.cpp" 
    "src/core/errors.cpp" 
    "src/renderer/camera_matrix_buffer.cpp" 
    "src/input/input_manager.cpp" 
    "src/player/flycam_controller.cpp"
    )

set(D3D12_BACKEND_SOURCE
    "src/rhi/d3d12/d3d12_render_device.hpp"
    "src/rhi/d3d12/d3d12_render_device.cpp"

    "src/rhi/d3d12/helpers.hpp"
    "src/rhi/d3d12/helpers.cpp"
    "src/rhi/d3d12/d3d12_command_list.cpp"
    "src/rhi/d3d12/d3d12_resource_command_list.cpp"
    "src/rhi/d3d12/d3d12_compute_command_list.cpp"
    "src/rhi/d3d12/d3d12_render_command_list.cpp"
    "src/rhi/d3d12/d3d12_descriptor_allocator.cpp" 
    "src/rhi/d3d12/shader_reflection.cpp" 
    "src/rhi/d3d12/d3d12_bind_group.cpp"
    )

# TODO: If I ever support multiple D3D12_BACKEND_SOURCE, only add the source for the selected backend
set(SANITY_ENGINE_SOURCE "${SANITY_ENGINE_SOURCE}" "${D3D12_BACKEND_SOURCE}")

add_executable(SanityEngine ${SANITY_ENGINE_SOURCE})

target_include_directories(SanityEngine PRIVATE ${CMAKE_CURRENT_LIST_DIR}/extern/minitrace)
target_include_directories(SanityEngine PRIVATE ${CMAKE_CURRENT_LIST_DIR}/extern/D3D12MemoryAllocator/src)

target_compile_definitions(SanityEngine PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
target_compile_definitions(SanityEngine PRIVATE MTR_ENABLED)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(SanityEngine PRIVATE -Wno-deprecated-declarations -Wno-documentation)
endif()

# Hook up dependencies

target_include_directories(SanityEngine PRIVATE "${CMAKE_CURRENT_LIST_DIR}/extern/DirectXMath/Inc")
target_include_directories(SanityEngine PRIVATE "${CMAKE_CURRENT_LIST_DIR}/extern/glfw/include")
target_link_libraries(SanityEngine spdlog glfw d3d12 dxgi d3dcompiler EnTT glm)

# Compile shaders

find_package(Python3)
if(NOT Python3_Interpreter_FOUND)
    message(ERROR "Could not find Python3 interpreter")
endif()

add_custom_target(CompileShaders
    COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_LIST_DIR}/tools/compile_shaders.py" "${CMAKE_CURRENT_LIST_DIR}/data/shaders" "${CMAKE_BINARY_DIR}/SanityEngine/data/shaders"
    COMMENT "Compiling shaders"
    VERBATIM
)

add_dependencies(SanityEngine CompileShaders)
