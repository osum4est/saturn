cmake_minimum_required(VERSION 3.19)

option(SATURN_BUILD_TESTING "Build tests" OFF)
option(SATURN_BUILD_EXAMPLES "Build examples" OFF)

include(FetchContent)
FetchContent_Declare(
        xgraphics
        GIT_REPOSITORY https://github.com/osum4est/xgraphics
        GIT_TAG 912fc1c45d51842cce4b4934f8d8815a0bb394cb
)

FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw
        GIT_TAG 3.3.8
)

project(saturn)
set(TARGET_NAME ${PROJECT_NAME})
set(CMAKE_CXX_STANDARD 20)

add_library(${TARGET_NAME}
        include/saturn/ecs/ecs_core.hpp
        include/saturn/saturn.h
        src/ecs/ecs_core.cpp
        include/saturn/ecs/entity.hpp
        include/saturn/ecs/ecs.h
        include/saturn/ecs/universe.hpp
        include/saturn/ecs/world.hpp
        include/saturn/ecs/query.hpp
        include/saturn/ecs/utils/component_pool.hpp
        include/saturn/ecs/ecs_types.h
        include/saturn/ecs/component.hpp
        include/saturn/ecs/system.hpp
        include/saturn/ecs/stage.h
        include/saturn/ecs/trait_helpers.h
        include/saturn/window/window.h
        src/window/window.cpp)

target_include_directories(${TARGET_NAME} PUBLIC include)

FetchContent_MakeAvailable(xgraphics)
target_link_libraries(${TARGET_NAME} PRIVATE xgraphics)
target_link_libraries(${TARGET_NAME} PUBLIC result)

FetchContent_MakeAvailable(glfw)
target_link_libraries(${TARGET_NAME} PRIVATE glfw)

if ((CMAKE_PROJECT_NAME STREQUAL TARGET_NAME OR SATURN_BUILD_TESTING) AND BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif ()

if ((CMAKE_PROJECT_NAME STREQUAL TARGET_NAME OR SATURN_BUILD_EXAMPLES))
    add_subdirectory(examples)
endif ()

if (APPLE)
    target_compile_definitions(${TARGET_NAME} PUBLIC SATURN_MACOS)
elseif (UNIX)
    target_compile_definitions(${TARGET_NAME} PUBLIC SATURN_LINUX)
endif ()