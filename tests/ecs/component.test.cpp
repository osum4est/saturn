#include <catch2/catch_test_macros.hpp>
#include <saturn/saturn.h>

struct component_a {
    int a;
};

struct component_b {
    int b;
};

TEST_CASE("component", "[ecs]") {
    auto universe = saturn::universe::create();
    auto world = universe->create_world();
    auto entity = world->create_entity();
    auto component = entity.add<component_a>().get();

    SECTION("check if component is equal to itself") {
        REQUIRE(component == component);
    }

    SECTION("check if component is equal to a copy of itself") {
        auto copy = component;
        REQUIRE(component == copy);
    }

    SECTION("check if component is equal to a changed component") {
        auto other = entity.set<component_a>({5}).get();
        REQUIRE(component == other);
    }

    SECTION("check if component is equal to getting the component") {
        auto other = entity.get<component_a>().get();
        REQUIRE(component == other);
    }

    SECTION("check if component is equal to a component on a different entity") {
        auto entity_2 = world->create_entity();
        auto component_2 = entity_2.add<component_a>().get();
        REQUIRE(component != component_2);
    }
}
