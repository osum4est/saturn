#include <catch2/catch_test_macros.hpp>
#include <saturn/saturn.h>
#include <utility>

struct component_a {
    int a;
    component_a() : a(10) { }
    explicit component_a(int a) : a(a) { }
};

struct component_b {
    int b;
    component_b() : b(20) { }
    explicit component_b(int b) : b(b) { }
};

struct child {
    int x, y;
    int r, g, b;
};

struct component_complex {
    std::string str;
    std::vector<int> vec;
    uint64_t num;
    child c;
    component_complex() : vec({}), num(0), c({0, 0, 0, 0, 0}) { }
    component_complex(std::string str, std::vector<int> vec, uint64_t num, child c)
        : str(std::move(str)), vec(std::move(vec)), num(num), c(c) { }
};

TEST_CASE("entity", "[ecs]") {
    auto universe = saturn::universe::create();
    auto world = universe->create_world();
    auto entity = world->create_entity();

    component_a test_a_1(1);
    component_a test_a_2(2);
    component_b test_b_1(3);
    component_complex test_complex_1 = {"hello", {1, 2, 3}, 100, {1, 2, 3, 4, 5}};

    SECTION("add a component") {
        auto component = entity.add<component_a>().get();
        REQUIRE(component->a == 10);
    }

    SECTION("add a component with arguments") {
        auto component = entity.add<component_a>(100).get();
        REQUIRE(component->a == 100);
    }

    SECTION("add a pointer component") {
        auto component = entity.add<component_a*>().get();
        REQUIRE(*component == nullptr);
    }

    SECTION("add a pointer component with arguments") {
        auto ptr = new component_a(100);
        auto component = *entity.add<component_a*>(ptr).get();
        REQUIRE(component == ptr);

        // Even if the entity archetype changes, the pointer should still be the same
        entity.add<component_b>();
        REQUIRE(component == ptr);
        delete ptr;
    }

    SECTION("add a complex component") {
        auto component = entity.add<component_complex>().get();
        REQUIRE(component->str.empty());
        REQUIRE(component->vec.empty());
        REQUIRE(component->num == 0);
        REQUIRE(component->c.x == 0);
        REQUIRE(component->c.y == 0);
        REQUIRE(component->c.r == 0);
        REQUIRE(component->c.g == 0);
        REQUIRE(component->c.b == 0);
    }

    SECTION("add a complex component with arguments") {
        auto component =
            entity.add<component_complex>("hello", std::vector<int> {1, 2, 3}, 100, child {1, 2, 3, 4, 5}).get();
        REQUIRE(component->str == "hello");
        REQUIRE(component->vec.size() == 3);
        REQUIRE(component->vec[0] == 1);
        REQUIRE(component->vec[1] == 2);
        REQUIRE(component->vec[2] == 3);
        REQUIRE(component->num == 100);
        REQUIRE(component->c.x == 1);
        REQUIRE(component->c.y == 2);
        REQUIRE(component->c.r == 3);
        REQUIRE(component->c.g == 4);
        REQUIRE(component->c.b == 5);
    }

    SECTION("set a component") {
        auto component = entity.set<component_a>(test_a_1).get();
        REQUIRE(component->a == test_a_1.a);
    }

    SECTION("set a pointer component") {
        auto ptr = new component_a(test_a_1);
        auto component = *entity.set<component_a*>(ptr).get();
        REQUIRE(component == ptr);

        // Even if the entity archetype changes, the pointer should still be the same
        entity.add<component_b>();
        REQUIRE(component == ptr);
        delete ptr;
    }

    SECTION("set a complex component") {
        auto component = entity.set<component_complex>(test_complex_1).get();
        REQUIRE(component->str == test_complex_1.str);
        REQUIRE(component->vec == test_complex_1.vec);
        REQUIRE(component->num == test_complex_1.num);
        REQUIRE(component->c.x == test_complex_1.c.x);
        REQUIRE(component->c.y == test_complex_1.c.y);
        REQUIRE(component->c.r == test_complex_1.c.r);
        REQUIRE(component->c.g == test_complex_1.c.g);
        REQUIRE(component->c.b == test_complex_1.c.b);
    }

    SECTION("set a complex component by value") {
        auto component = entity
                             .set<component_complex>(
                                 component_complex {"hello", std::vector<int> {1, 2, 3}, 100, child {1, 2, 3, 4, 5}})
                             .get();
        REQUIRE(component->str == "hello");
        REQUIRE(component->vec.size() == 3);
        REQUIRE(component->vec[0] == 1);
        REQUIRE(component->vec[1] == 2);
        REQUIRE(component->vec[2] == 3);
        REQUIRE(component->num == 100);
        REQUIRE(component->c.x == 1);
        REQUIRE(component->c.y == 2);
        REQUIRE(component->c.r == 3);
        REQUIRE(component->c.g == 4);
        REQUIRE(component->c.b == 5);
    }

    SECTION("has a component that was added") {
        entity.add<component_a>().get();
        REQUIRE(entity.has<component_a>());
    }

    SECTION("has a component that was set") {
        entity.set<component_a>(test_a_1).get();
        REQUIRE(entity.has<component_a>());
    }

    SECTION("has a missing component") {
        REQUIRE(!entity.has<component_a>());
    }

    SECTION("has a missing component after adding a component") {
        entity.add<component_a>().get();
        REQUIRE(!entity.has<component_b>());
    }

    SECTION("get a component") {
        auto added_component = entity.add<component_a>(test_a_1).get();
        auto component = entity.get<component_a>().get();
        REQUIRE(component->a == test_a_1.a);
    }

    SECTION("add two of the same types of components") {
        auto component_1 = entity.add<component_a>().get();
        REQUIRE(entity.add<component_a>().is_err());
    }

    SECTION("add two different components") {
        auto component_1 = entity.add<component_a>().get();
        auto component_2 = entity.add<component_b>().get();
        REQUIRE(component_1->a == 10);
        REQUIRE(component_2->b == 20);
        REQUIRE((void*) &*component_1 != (void*) &*component_2);
        REQUIRE(entity.has<component_a>());
        REQUIRE(entity.has<component_b>());
    }

    SECTION("set two of the same types of components") {
        auto component_1 = entity.set<component_a>(test_a_1).get();
        auto component_2 = entity.set<component_a>(test_a_2).get();
        REQUIRE(component_2->a != test_a_1.a);
        REQUIRE(component_2->a == test_a_2.a);
        REQUIRE(&*component_1 == &*component_2);
        REQUIRE(entity.has<component_a>());
    }

    SECTION("set two different components") {
        auto component_1 = entity.set<component_a>(test_a_1).get();
        auto component_2 = entity.set<component_b>(test_b_1).get();
        REQUIRE(component_1->a == test_a_1.a);
        REQUIRE(component_2->b == test_b_1.b);
        REQUIRE((void*) &*component_1 != (void*) &*component_2);
        REQUIRE(entity.has<component_a>());
        REQUIRE(entity.has<component_b>());
    }

    SECTION("change a value in a component returned from add") {
        auto component = entity.add<component_a>().get();
        component->a = 5;
        REQUIRE(entity.get<component_a>().get()->a == 5);
    }

    SECTION("change a value in a component returned from set") {
        auto component = entity.set<component_a>(test_a_1).get();
        component->a = 5;
        REQUIRE(entity.get<component_a>().get()->a == 5);
    }

    SECTION("change a value in a component returned from get") {
        auto component = entity.set<component_a>(test_a_1).get();
        entity.get<component_a>().get()->a = 5;
        REQUIRE(component->a == 5);
    }

    SECTION("remove a component") {
        entity.set<component_a>(test_a_1).get();
        entity.remove<component_a>();
        REQUIRE(!entity.has<component_a>());
    }

    SECTION("remove a component on an entity with different component") {
        entity.set<component_a>(test_a_1).get();
        entity.set<component_b>(test_b_1).get();
        entity.remove<component_a>();
        REQUIRE(!entity.has<component_a>());
        REQUIRE(entity.has<component_b>());
        REQUIRE(entity.get<component_b>().get()->b == test_b_1.b);
    }

    SECTION("remove a missing component") {
        REQUIRE_NOTHROW(entity.remove<component_a>());
    }

    SECTION("remove a removed component") {
        entity.set<component_a>(test_a_1).get();
        entity.remove<component_a>();
        REQUIRE_NOTHROW(entity.remove<component_a>());
    }

    SECTION("remove a missing component from an entity with a different component") {
        entity.set<component_a>(test_a_1).get();
        entity.remove<component_b>();
    }

    SECTION("get a missing component") {
        REQUIRE(entity.get<component_a>().is_err());
    }

    SECTION("get a removed component") {
        entity.set<component_a>(test_a_1).get();
        entity.remove<component_a>();
        REQUIRE(entity.get<component_a>().is_err());
    }

    SECTION("get a missing component from an entity with a different component") {
        entity.set<component_a>(test_a_1).get();
        REQUIRE(entity.get<component_b>().is_err());
    }

    SECTION("get a removed component from an entity with a different component") {
        entity.set<component_a>(test_a_1).get();
        entity.set<component_b>(test_b_1).get();
        entity.remove<component_a>();
        REQUIRE(entity.get<component_a>().is_err());
    }

    SECTION("set a removed component") {
        entity.set<component_a>(test_a_1).get();
        entity.remove<component_a>();
        auto component = entity.set<component_a>(test_a_2).get();
        REQUIRE(component->a == test_a_2.a);
    }

    SECTION("set value in removed component") {
        auto component = entity.set<component_a>(test_a_1).get();
        entity.remove<component_a>();
        REQUIRE_THROWS(component->a = 5);
    }

    SECTION("assign to removed component") {
        auto component = entity.set<component_a>(test_a_1).get();
        entity.remove<component_a>();
        REQUIRE_THROWS(*component = test_a_2);
    }

    SECTION("set value in component on destroyed entity") {
        auto component = entity.set<component_a>(test_a_1).get();
        world->destroy_entity(entity);
        REQUIRE_THROWS(component->a = 5);
    }

    SECTION("assign to component on destroyed entity") {
        auto component = entity.set<component_a>(test_a_1).get();
        world->destroy_entity(entity);
        REQUIRE_THROWS(*component = test_a_2);
    }
}

TEST_CASE("multiple entities", "[ecs]") {
    auto universe = saturn::universe::create();
    auto world = universe->create_world();
    std::vector<saturn::entity> entities = {};
    for (int i = 0; i < 100; i++)
        entities.push_back(world->create_entity());

    component_a test_a_1(1);
    component_complex test_complex_1 = {};

    SECTION("compare entities") {
        REQUIRE(entities[0] == entities[0]);
        REQUIRE(entities[0] != entities[1]);
    }

    SECTION("add components to each entity") {
        for (auto& entity : entities) {
            entity.add<component_a>(3).get();
            entity.add<component_complex>().get();
        }
        for (auto& entity : entities) {
            REQUIRE(entity.get<component_a>().get()->a == 3);
            REQUIRE(entity.get<component_complex>().get()->num == 0);
            REQUIRE(!entity.has<component_b>());
        }
    }

    SECTION("set components on entity") {
        for (auto& entity : entities) {
            entity.set<component_a>(test_a_1).get();
            entity.set<component_complex>(test_complex_1).get();
        }
        for (auto& entity : entities) {
            REQUIRE(entity.get<component_a>().get()->a == test_a_1.a);
            REQUIRE(entity.get<component_complex>().get()->num == test_complex_1.num);
            REQUIRE(!entity.has<component_b>());
        }
    }

    SECTION("add components with unique data to each entity") {
        int i = 0;
        for (auto& entity : entities) {
            entity.add<component_a>(i).get();
            entity.add<component_b>(1000 + i).get();
            i++;
        }

        i = 0;
        for (auto& entity : entities) {
            REQUIRE(entity.get<component_a>().get()->a == i);
            REQUIRE(entity.get<component_b>().get()->b == 1000 + i);
            i++;
        }
    }

    SECTION("set components with unique data on each entity") {
        int i = 0;
        for (auto& entity : entities) {
            entity.set<component_a>(component_a(i)).get();
            entity.set<component_b>(component_b(1000 + i)).get();
            i++;
        }

        i = 0;
        for (auto& entity : entities) {
            REQUIRE(entity.get<component_a>().get()->a == i);
            REQUIRE(entity.get<component_b>().get()->b == 1000 + i);
            i++;
        }
    }

    SECTION("remove components from each entity") {
        for (auto& entity : entities) {
            entity.add<component_a>().get();
            entity.add<component_complex>().get();
        }
        for (auto& entity : entities) {
            entity.remove<component_a>();
            entity.remove<component_complex>();
        }
        for (auto& entity : entities) {
            REQUIRE(!entity.has<component_a>());
            REQUIRE(!entity.has<component_complex>());
        }
    }

    SECTION("remove component_a from each entity") {
        for (auto& entity : entities) {
            entity.add<component_a>().get();
            entity.add<component_complex>().get();
        }
        for (auto& entity : entities) {
            entity.remove<component_a>();
        }
        for (auto& entity : entities) {
            REQUIRE(!entity.has<component_a>());
            REQUIRE(entity.has<component_complex>());
            REQUIRE(entity.get<component_complex>().get()->num == 0);
        }
    }

    SECTION("remove complex_component from each entity") {
        for (auto& entity : entities) {
            entity.add<component_a>(3).get();
            entity.add<component_complex>().get();
        }
        for (auto& entity : entities) {
            entity.remove<component_complex>();
        }
        for (auto& entity : entities) {
            REQUIRE(entity.has<component_a>());
            REQUIRE(entity.get<component_a>().get()->a == 3);
            REQUIRE(!entity.has<component_complex>());
        }
    }
}

TEST_CASE("destroyed entity", "[ecs]") {
    auto universe = saturn::universe::create();
    auto world = universe->create_world();
    auto entity = world->create_entity();
    entity.add<component_a>();
    world->destroy_entity(entity);

    SECTION("add component") {
        REQUIRE(entity.add<component_a>(3).is_err());
    }

    SECTION("set component") {
        REQUIRE(entity.set<component_a>({}).is_err());
    }

    SECTION("remove component") {
        REQUIRE_NOTHROW(entity.remove<component_a>());
    }

    SECTION("get component") {
        REQUIRE(entity.get<component_a>().is_err());
    }

    SECTION("has component") {
        REQUIRE(!entity.has<component_a>());
    }
}

TEST_CASE("destroyed world", "[ecs]") {
    auto universe = saturn::universe::create();
    auto world = universe->create_world();
    auto entity = world->create_entity();
    auto component = entity.add<component_a>().get();
    world.reset();

    SECTION("check if entity is alive") {
        REQUIRE(!entity.alive());
        REQUIRE(entity.dead());
    }

    SECTION("add component") {
        REQUIRE(entity.add<component_a>(3).is_err());
    }

    SECTION("set component") {
        REQUIRE(entity.set<component_a>({}).is_err());
    }

    SECTION("remove component") {
        REQUIRE_NOTHROW(entity.remove<component_a>());
    }

    SECTION("get component") {
        REQUIRE(entity.get<component_a>().is_err());
    }

    SECTION("has component") {
        REQUIRE(!entity.has<component_a>());
    }

    SECTION("set value in component") {
        REQUIRE_THROWS(component->a == 0);
    }

    SECTION("get value in component") {
        REQUIRE_THROWS(*component);
    }
}
