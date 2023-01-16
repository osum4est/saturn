#include <catch2/catch_test_macros.hpp>
#include <saturn/saturn.h>

struct component_a {
    int a;
};

struct component_b {
    int b;
};

struct component_c {
    int c;
};

struct struct_system : public saturn::system<component_a, const component_b> {
    void run(ctx_t& ctx, query_t& query) override {
        for (auto [entity, a, b] : query) {
            a.a = 100 + b.b;
        }
    }
};

TEST_CASE("system", "[ecs]") {
    auto universe = saturn::universe::create();
    auto world = universe->create_world();

    std::unordered_map<saturn::entity_id, saturn::entity> entities;
    for (int i = 0; i < 10; i++) {
        auto entity = world->create_entity();
        if (i % 2 == 0) entity.set<component_a>({i});
        if (i % 3 == 0) entity.set<component_b>({i});
        if (i % 5 == 0) entity.set<component_c>({i});
        entities[entity.id()] = entity;
    }

    int called = 0;
    int results = 0;

    SECTION("update with no systems") {
        world->update();
        REQUIRE(called == 0);
    }

    SECTION("system with no components") {
        world->create_system([&](auto& query) {
            called++;
            results = query.count();
        });
        world->update();
        REQUIRE(called == 1);
        REQUIRE(results == 10);
    }

    SECTION("system with one component") {
        world->create_system<component_a>([&](auto& query) {
            called++;
            results = query.count();
        });
        world->update();
        REQUIRE(called == 1);
        REQUIRE(results == 5);
    }

    SECTION("system with two components") {
        world->create_system<component_a, component_b>([&](auto& query) {
            called++;
            results = query.count();
        });
        world->update();
        REQUIRE(called == 1);
        REQUIRE(results == 2);
    }

    SECTION("system with three components") {
        world->create_system<component_a, component_b, component_c>([&](auto& query) {
            called++;
            results = query.count();
        });
        world->update();
        REQUIRE(called == 1);
        REQUIRE(results == 1);
    }

    SECTION("system with context") {
        saturn::delta_time dt = 0.0f;
        world->create_system<component_a>([&](auto& ctx, auto& query) {
            called++;
            dt = ctx.dt();
        });
        world->update();
        REQUIRE(called == 1);
        REQUIRE(dt > 0.0f);
    }

    SECTION("systems with different stages") {
        int stage = 0;
        world->create_system<component_a>(saturn::stages::post_update, [&](auto& query) {
            called++;
            REQUIRE(stage == 2);
            stage++;
        });
        world->create_system<component_a>(saturn::stages::update, [&](auto& query) {
            called++;
            REQUIRE(stage == 1);
            stage++;
        });
        world->create_system<component_a>(saturn::stages::pre_update, [&](auto& query) {
            called++;
            REQUIRE(stage == 0);
            stage++;
        });
        world->update();
        REQUIRE(called == 3);
    }

    SECTION("struct system") {
        world->create_system<struct_system>();
        world->update();
        for (auto [id, entity] : entities) {
            if (entity.has<component_a>() && entity.has<component_b>()) {
                auto a = entity.get<component_a>().get();
                auto b = entity.get<component_b>().get();
                REQUIRE(a->a == 100 + b->b);
            }
        }
    }

    SECTION("update system multiple times") {
        world->create_system<component_a>([&](auto& query) {
            called++;
            results += query.count();
        });
        for (int i = 0; i < 10; i++)
            world->update();
        REQUIRE(called == 10);
        REQUIRE(results == 50);
    }

    SECTION("destroy system") {
        auto system = world->create_system<component_a>([&](auto& query) {
            called++;
            results = query.count();
        });
        world->update();
        REQUIRE(called == 1);
        REQUIRE(results == 5);
        world->destroy_system(system);
        world->update();
        REQUIRE(called == 1);
        REQUIRE(results == 5);
    }

    SECTION("destroy destroyed system") {
        auto system = world->create_system<component_a>([&](auto& query) {});
        world->destroy_system(system);
        world->destroy_system(system);
    }

    SECTION("destroy invalid system") {
        world->destroy_system(0);
    }
}
