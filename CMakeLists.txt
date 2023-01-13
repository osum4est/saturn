cmake_minimum_required(VERSION 3.19)

option(SATURN_BUILD_TESTING "Build tests" OFF)

include(FetchContent)
FetchContent_Declare(
        xgraphics
        GIT_REPOSITORY https://github.com/osum4est/xgraphics
        GIT_TAG 912fc1c45d51842cce4b4934f8d8815a0bb394cb
)

project(saturn)
set(TARGET_NAME ${PROJECT_NAME})
set(CMAKE_CXX_STANDARD 20)

add_library(${TARGET_NAME}
        include/saturn/ecs/ecs.hpp
        include/saturn/saturn.h
        src/ecs/ecs.cpp)

target_include_directories(${TARGET_NAME} PUBLIC include)

FetchContent_MakeAvailable(xgraphics)
target_link_libraries(${TARGET_NAME} PRIVATE xgraphics)
target_link_libraries(${TARGET_NAME} PUBLIC result)

if ((CMAKE_PROJECT_NAME STREQUAL TARGET_NAME OR SATURN_BUILD_TESTING) AND BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif ()
