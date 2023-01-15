#include <catch2/catch_test_macros.hpp>
#include <saturn/saturn.h>

TEST_CASE("universe", "[ecs]") {
    auto universe = saturn::universe::create();

    SECTION("create world") {
        auto world = universe->create_world();
    }
}