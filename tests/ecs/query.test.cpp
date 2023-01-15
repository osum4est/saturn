#include <catch2/catch_test_macros.hpp>
#include <saturn/ecs/ecs.hpp>
#include <unordered_set>

struct test_component_a {
    int a;
};

struct test_component_b {
    int b;
};

struct test_component_c {
    int c;
};

TEST_CASE("query", "[ecs]") {
    auto universe = saturn::universe::create();
    auto world = universe->create_world();

    SECTION("query a world with no entities for entities with any components") {
        auto query = world->create_query();
        REQUIRE(query.begin() == query.end());
        REQUIRE(query.count() == 0);
    }

    SECTION("query a world with no entities for entities with a component") {
        auto query = world->create_query<test_component_a>();
        REQUIRE(query.count() == 0);
    }

    SECTION("add entities to the world") {
        std::unordered_map<saturn::entity_id, saturn::entity> entities;
        for (int i = 0; i < 10; i++) {
            auto entity = world->create_entity();
            if (i % 2 == 0) entity.set<test_component_a>({i});
            if (i % 3 == 0) entity.set<test_component_b>({i});
            if (i % 5 == 0) entity.set<test_component_c>({i});
            entities[entity.id()] = entity;
        }

        SECTION("query for entities with any components") {
            auto query = world->create_query();
            std::unordered_set<saturn::entity_id> ids;
            REQUIRE(query.count() == 10);
            for (auto [entity] : query) {
                REQUIRE(entity.alive());
                REQUIRE(entities.contains(entity.id()));
                REQUIRE(!ids.contains(entity.id()));
                ids.insert(entity.id());
            }
        }

        SECTION("query for entities with test_component_a") {
            auto query = world->create_query<test_component_a>();
            std::unordered_set<saturn::entity_id> ids;
            REQUIRE(query.count() == 5);
            for (const auto& [entity, component_a] : query) {
                REQUIRE(entities.contains(entity.id()));
                REQUIRE(entity.has<test_component_a>());
                REQUIRE(component_a.a == entities[entity.id()].get<test_component_a>().get()->a);
                REQUIRE(&component_a == &*entities[entity.id()].get<test_component_a>().get());
                REQUIRE(!ids.contains(entity.id()));
                ids.insert(entity.id());
            }
        }

        SECTION("query for entities with test_component_b") {
            auto query = world->create_query<test_component_b>();
            REQUIRE(query.count() == 4);
            for (const auto& [entity, component_b] : query) {
                REQUIRE(entities.contains(entity.id()));
                REQUIRE(entity.has<test_component_b>());
            }
        }

        SECTION("query for entities with test_component_c") {
            auto query = world->create_query<test_component_c>();
            REQUIRE(query.count() == 2);
            for (const auto& [entity, component_c] : query) {
                REQUIRE(entities.contains(entity.id()));
                REQUIRE(entity.has<test_component_c>());
            }
        }

        SECTION("query for entities with test_component_a and test_component_b") {
            auto query = world->create_query<test_component_a, test_component_b>();
            REQUIRE(query.count() == 2);
            for (const auto& [entity, component_a, component_b] : query) {
                REQUIRE(entities.contains(entity.id()));
                REQUIRE(entity.has<test_component_a>());
                REQUIRE(entity.has<test_component_b>());
            }
        }

        SECTION("query for entities with test_component_a and test_component_c") {
            auto query = world->create_query<test_component_a, test_component_c>();
            REQUIRE(query.count() == 1);
            for (const auto& [entity, component_a, component_c] : query) {
                REQUIRE(entities.contains(entity.id()));
                REQUIRE(entity.has<test_component_a>());
                REQUIRE(entity.has<test_component_c>());
            }
        }

        SECTION("query for entities with test_component_b and test_component_c") {
            auto query = world->create_query<test_component_b, test_component_c>();
            REQUIRE(query.count() == 1);
            for (const auto& [entity, component_b, component_c] : query) {
                REQUIRE(entities.contains(entity.id()));
                REQUIRE(entity.has<test_component_b>());
                REQUIRE(entity.has<test_component_c>());
            }
        }

        SECTION("query for entities with test_component_a, test_component_b, and test_component_c") {
            auto query = world->create_query<test_component_a, test_component_b, test_component_c>();
            REQUIRE(query.count() == 1);
            for (const auto& [entity, component_a, component_b, component_c] : query) {
                REQUIRE(entities.contains(entity.id()));
                REQUIRE(entity.has<test_component_a>());
                REQUIRE(entity.has<test_component_b>());
                REQUIRE(entity.has<test_component_c>());
            }
        }

        SECTION("query for entities with test_component_a and const test_component_b") {
            auto query = world->create_query<test_component_a, const test_component_b>();
            REQUIRE(query.count() == 2);
            for (const auto& [entity, component_a, component_b] : query) {
                REQUIRE(entities.contains(entity.id()));
                REQUIRE(entity.has<test_component_a>());
                REQUIRE(entity.has<test_component_b>());
            }
        }
    }
}