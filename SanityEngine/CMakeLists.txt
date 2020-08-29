cmake_minimum_required(VERSION 3.14)
project(SanityEngine)

############################
# Initialize cmake options #
############################
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Force VS to use C++20
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest /arch:AVX2 /experimental:module")

# Global compile definitions that make my life
add_compile_definitions(
    TRACY_ENABLE
    RX_DEBUG
    GLM_ENABLE_EXPERIMENTAL
    _CRT_SECURE_NO_WARNINGS
    GLM_FORCE_LEFT_HANDED
    NOMINMAX
    WIN32_LEAN_AND_MEAN
)

######################
# VCPKG dependencies #
######################
find_package(assimp CONFIG REQUIRED)
find_package(entt CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

######################
# Other dependencies #
######################
set(THIRD_PARTY_DIR ${CMAKE_CURRENT_LIST_DIR}/extern)

set(D3D12MA_SOURCE 
    ${THIRD_PARTY_DIR}/D3D12MemoryAllocator/D3D12MemAlloc.cpp
    )
source_group(TREE ${THIRD_PARTY_DIR} FILES ${D3D12MA_SOURCE})

set(TRACY_SOURCE 
    ${THIRD_PARTY_DIR}/tracy/TracyClient.cpp
    )
source_group(TREE ${THIRD_PARTY_DIR} FILES ${TRACY_SOURCE})

set(IMGUI_SOURCE
    ${THIRD_PARTY_DIR}/tracy/imgui/imgui.cpp 
    ${THIRD_PARTY_DIR}/tracy/imgui/imgui_draw.cpp 
    ${THIRD_PARTY_DIR}/tracy/imgui/imgui_widgets.cpp 
    ${THIRD_PARTY_DIR}/tracy/imgui/imgui_demo.cpp 
    )
source_group(TREE ${THIRD_PARTY_DIR} FILES ${IMGUI_SOURCE})

file(GLOB_RECURSE REX_SOURCE 
     LIST_DIRECTORIES false
     CONFIGURE_DEPENDS
     ${THIRD_PARTY_DIR}/rex/include/**
     )
source_group(TREE ${THIRD_PARTY_DIR}/rex/include FILES ${REX_SOURCE})

add_subdirectory(runtimeclasses)

#########################
# Compile Sanity Engine #
#########################
file(GLOB_RECURSE SANITY_ENGINE_SOURCE 
     LIST_DIRECTORIES false
     CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_LIST_DIR}/src/**
     )

source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${SANITY_ENGINE_SOURCE})

add_executable(SanityEngine ${SANITY_ENGINE_SOURCE} ${REX_SOURCE} ${D3D12MA_SOURCE} ${IMGUI_SOURCE} ${TRACY_SOURCE})

target_include_directories(SanityEngine PRIVATE 
    ${THIRD_PARTY_DIR}/physx/include
    ${THIRD_PARTY_DIR}/physx/include/physx
    ${THIRD_PARTY_DIR}/rex/include
    ${THIRD_PARTY_DIR}/tracy
    ${THIRD_PARTY_DIR}/tracy/imgui
    ${THIRD_PARTY_DIR}/json5/include
    ${THIRD_PARTY_DIR}/dotnet/include
    ${THIRD_PARTY_DIR}/D3D12MemoryAllocator
    ${THIRD_PARTY_DIR}/bve/include
    ${THIRD_PARTY_DIR}/pix/include
    ${CMAKE_CURRENT_LIST_DIR}/src
    )
target_link_directories(SanityEngine PRIVATE
    ${THIRD_PARTY_DIR}/rex/include
    ${THIRD_PARTY_DIR}/physx/lib/debug
    ${THIRD_PARTY_DIR}/pix/lib
    ${THIRD_PARTY_DIR}/bve/lib
    ${THIRD_PARTY_DIR}/pix/lib
    )
target_link_libraries(SanityEngine PRIVATE
    dxguid
    WindowsApp
    PhysX_64
    PhysXCommon_64
    PhysXFoundation_64
    assimp::assimp
    bve_native.dll.lib
    EnTT::EnTT
    d3d12
    dxgi
    d3dcompiler
    WinPixEventRuntime
    glfw
    )

find_path(CPPWINRT_INCLUDE_DIRS "winrt/Windows.ApplicationModel.Activation.h")
target_include_directories(SanityEngine PRIVATE ${CPPWINRT_INCLUDE_DIRS})

find_path(STB_INCLUDE_DIRS "stb.h")
target_include_directories(SanityEngine PRIVATE ${STB_INCLUDE_DIRS})

find_path(NETHOST_INCLUDE_DIRS nethost.h)
target_include_directories(SanityEngine PRIVATE ${NETHOST_INCLUDE_DIRS})

find_library(NETHOST_LIBRARY NAMES libnethost nethost)
target_link_libraries(SanityEngine PRIVATE ${NETHOST_LIBRARY})

# target_link_libraries(SanityEngine PRIVATE SanityEngineRuntimeApi)

###################
# Compile shaders #
###################
find_package(Python3)
if(NOT Python3_Interpreter_FOUND)
    message(ERROR "Could not find Python3 interpreter")
endif()

add_custom_command(TARGET SanityEngine
    PRE_BUILD
    COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_LIST_DIR}/tools/compile_shaders.py" "${CMAKE_CURRENT_LIST_DIR}/data/shaders" "${CMAKE_CURRENT_BINARY_DIR}/data/shaders"
)
