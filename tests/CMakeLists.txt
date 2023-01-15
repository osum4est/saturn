set(TARGET_NAME ${PROJECT_NAME}-tests)

add_executable(${TARGET_NAME} ecs/universe.test.cpp ecs/world.test.cpp ecs/entity.test.cpp ecs/query.test.cpp ecs/component.test.cpp)
target_link_libraries(${TARGET_NAME} PRIVATE ${PROJECT_NAME})
target_include_directories(${TARGET_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/src)

find_package(Catch2 3 REQUIRED)
target_link_libraries(${TARGET_NAME} PRIVATE Catch2::Catch2WithMain)
include(CTest)
include(Catch)
catch_discover_tests(${TARGET_NAME})
