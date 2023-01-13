#include <catch2/catch_test_macros.hpp>
#include <saturn/ecs/ecs.hpp>
#include <unordered_set>

TEST_CASE("world", "[ecs]") {
    auto universe = saturn::universe::create().get();
    auto world = universe->create_world().get();

    SECTION("create an entity") {
        auto entity = world->create_entity().get();
        REQUIRE(entity.alive());
        REQUIRE(!entity.dead());
    }

    SECTION("create 10 entities") {
        std::unordered_set<saturn::entity_id> ids = {};
        for (int i = 0; i < 10; i++) {
            auto entity = world->create_entity().get();
            REQUIRE(ids.find(entity.id()) == ids.end());
            ids.insert(entity.id());
        }
    }

    SECTION("destroy entity") {
        auto entity = world->create_entity().get();
        world->destroy_entity(entity);
        REQUIRE(!entity.alive());
        REQUIRE(entity.dead());
    }

    SECTION("destroy entity twice") {
        auto entity = world->create_entity().get();
        world->destroy_entity(entity);
        world->destroy_entity(entity);
        REQUIRE(!entity.alive());
        REQUIRE(entity.dead());
    }

    SECTION("create two entities and destroy one") {
        auto entity_1 = world->create_entity().get();
        auto entity_2 = world->create_entity().get();
        world->destroy_entity(entity_2);
        REQUIRE(entity_1.alive());
        REQUIRE(!entity_1.dead());
        REQUIRE(!entity_2.alive());
        REQUIRE(entity_2.dead());
    }

    SECTION("create 10 entities then destroy 10 entities") {
        std::vector<saturn::entity> entities;
        for (int i = 0; i < 10; i++)
            entities.push_back(world->create_entity().get());
        for (auto entity : entities)
            world->destroy_entity(entity);
    }

    SECTION("create and destroy 10 entities") {
        for (int i = 0; i < 10; i++) {
            auto entity = world->create_entity().get();
            REQUIRE(saturn::_::entity_id_index(entity.id()) == 0);
            REQUIRE(saturn::_::entity_id_version(entity.id()) == i);
            world->destroy_entity(entity);
        }
    }
}
