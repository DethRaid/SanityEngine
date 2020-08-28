############################
# Compile WinRT Components #
############################

file(GLOB_RECURSE SANITY_ENGINE_RUNTIME_SOURCE
    LIST_DIRECTORIES false
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/**)

source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${SANITY_ENGINE_RUNTIME_SOURCE})

add_library(SanityEngineRuntimeApi OBJECT ${SANITY_ENGINE_RUNTIME_SOURCE})
