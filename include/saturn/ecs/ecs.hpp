#ifndef SATURN_ECS_HPP
#define SATURN_ECS_HPP

#include <cstdlib>
#include <cstring>
#include <result/result.h>

namespace saturn {

typedef uint32_t array_index;
typedef uint32_t world_id;
typedef uint64_t entity_id;
typedef uint16_t component_id;

#define SATURN_ECS_MAX_COMPONENTS 64
typedef uint64_t archetype_mask;
typedef uint32_t archetype_id;
typedef uint64_t archetype_component_id;

// Hide this stuff from the user, they shouldn't need to use it
namespace _ {
array_index entity_id_index(entity_id id);
uint32_t entity_id_version(entity_id id);
entity_id create_entity_id(array_index index, uint32_t version);

uint32_t archetype_id_index(archetype_id id);
archetype_id create_archetype_id(array_index index);
archetype_component_id create_archetype_component_id(archetype_id archetype, component_id component);

array_index component_id_bit_index(component_id id);
component_id create_component_id(array_index bit_index);

bool archetype_mask_has_component(archetype_mask mask, component_id component);
archetype_mask archetype_mask_add_component(archetype_mask mask, component_id component);
archetype_mask archetype_mask_remove_component(archetype_mask mask, component_id component);

// TODO: Clean these up when the world is destroyed
class component_pool {
    component_id _component_id;
    size_t _component_size;
    size_t _component_count;
    void* _components = nullptr;
    size_t _capacity = 0;

  public:
    component_pool(component_id component_id, size_t component_size)
        : _component_id(component_id), _component_size(component_size), _component_count(0) {
        resize(64);
    }

    [[nodiscard]] component_id component_id() const {
        return _component_id;
    }

    [[nodiscard]] size_t size() const {
        return _component_count;
    }

    void add_component() {
        if (_component_count >= _capacity) resize(_capacity * 2);
        _component_count++;
    }

    void* operator[](array_index index) {
        return ((uint8_t*) _components) + index * _component_size;
    }

  private:
    void resize(size_t new_size) {
        _components = std::realloc(_components, new_size * _component_size);
        _capacity = new_size;
    }
};

struct archetype {
    archetype_id id;
    archetype_mask mask;
    std::vector<component_pool> component_pools;
};

struct entity_archetype {
    archetype_id archetype;
    array_index archetype_index;
};

struct ecs_data {
    std::vector<archetype> archetypes = {};
    std::vector<std::vector<array_index>> archetype_free_entities = {};
    std::unordered_map<archetype_mask, archetype_id> archetypes_by_mask = {};
    std::unordered_map<archetype_component_id, array_index> archetype_component_indices = {};

    std::vector<entity_id> entities = {};
    std::vector<array_index> free_entities = {};
    std::vector<entity_archetype> entity_archetypes = {};

    archetype_id empty_archetype_id = 0;
    array_index empty_archetype_index = 0;
    static std::vector<size_t> component_sizes;

    ecs_data() {
        const auto empty_archetype_mask = 0;
        auto& empty_archetype = get_or_create_archetype(empty_archetype_mask);
        empty_archetype_id = empty_archetype.id;
        empty_archetype_index = archetype_id_index(empty_archetype_id);
    }

    static component_id _next_component_id;
    template <typename T>
    component_id lookup_component_id() {
        static component_id id = 0;
        static bool initialized = false;
        if (initialized) return id;
        else {
            initialized = true;
            component_sizes.push_back(sizeof(T));
            return id = _next_component_id++;
        }
    }

    archetype& get_or_create_archetype(archetype_mask mask) {
        auto it = archetypes_by_mask.find(mask);
        if (it != archetypes_by_mask.end()) return archetypes[it->second];

        archetype_id id = create_archetype_id(archetypes.size());
        archetypes.push_back(archetype {.id = id, .mask = mask, .component_pools = {}});
        archetype_free_entities.emplace_back();
        archetype& archetype = archetypes.back();
        archetypes_by_mask[mask] = id;
        int archetype_component_index = 0;
        for (int i = 0; i < SATURN_ECS_MAX_COMPONENTS; i++) {
            if (!archetype_mask_has_component(archetype.mask, create_component_id(i))) continue;
            component_id component_id = create_component_id(i);
            archetype_component_id archetype_component_id = create_archetype_component_id(id, component_id);
            archetype_component_indices[archetype_component_id] = archetype_component_index;
            archetype.component_pools.emplace_back(component_id, component_sizes[i]);
            archetype_component_index++;
        }
        return archetype;
    }

    void* entity_archetype_component(entity_archetype& entity_archetype, component_id component) {
        archetype& archetype = archetypes[archetype_id_index(entity_archetype.archetype)];
        if (archetype.component_pools.empty()) return nullptr;

        archetype_component_id archetype_component_id = create_archetype_component_id(archetype.id, component);
        auto it = archetype_component_indices.find(archetype_component_id);
        if (it == archetype_component_indices.end()) return nullptr;
        return archetype.component_pools[it->second][entity_archetype.archetype_index];
    }

    void move_entity_to_archetype(entity_id entity, archetype& archetype) {
        entity_archetype& entity_archetype = entity_archetypes[entity_id_index(entity)];
        struct archetype& old_archetype = archetypes[archetype_id_index(entity_archetype.archetype)];
        if (old_archetype.id == archetype.id) return;

        array_index old_archetype_index = entity_archetype.archetype_index;
        remove_entity_from_archetype(entity);
        add_entity_to_archetype(entity, archetype);

        for (int i = 0; i < old_archetype.component_pools.size(); i++) {
            component_id id = old_archetype.component_pools[i].component_id();
            void* new_component = entity_archetype_component(entity_archetype, id);
            if (!new_component) continue; // If a component is being removed, then it won't be in the new archetype
            std::memcpy(new_component, old_archetype.component_pools[i][old_archetype_index],
                        component_sizes[component_id_bit_index(id)]);
        }
    }

    void remove_entity_from_archetype(entity_id entity) {
        entity_archetype& entity_archetype = entity_archetypes[entity_id_index(entity)];
        archetype& archetype = archetypes[archetype_id_index(entity_archetype.archetype)];

        // If were removing the entity from the empty archetype, then don't worry about free entities
        if (archetype.id == empty_archetype_id) {
            entity_archetype.archetype = -1;
            entity_archetype.archetype_index = -1;
            return;
        }

        archetype_free_entities[archetype_id_index(archetype.id)].push_back(entity_archetype.archetype_index);
        entity_archetype.archetype = -1;
        entity_archetype.archetype_index = -1;
    }

    void add_entity_to_archetype(entity_id entity, archetype& archetype) {
        entity_archetype& entity_archetype = entity_archetypes[entity_id_index(entity)];
        entity_archetype.archetype = archetype.id;

        // If were adding the entity to the empty archetype, then don't worry about free entities
        if (archetype.id == empty_archetype_id) {
            entity_archetype.archetype_index = 0;
            return;
        }

        auto& free_archetype_entites = archetype_free_entities[archetype_id_index(archetype.id)];
        if (free_archetype_entites.empty()) {
            entity_archetype.archetype_index = archetype.component_pools[0].size();
            for (auto& pool : archetype.component_pools)
                pool.add_component();
        } else {
            entity_archetype.archetype_index = free_archetype_entites.back();
            free_archetype_entites.pop_back();
        }
    }
};
} // namespace _

class entity;
template <typename T>
class component {
    friend class entity;

    component_id _id;
    entity_id _entity_id;
    _::ecs_data* _core;

    component(component_id id, entity_id entity_id, _::ecs_data* core) : _id(id), _entity_id(entity_id), _core(core) { }

  public:
    [[nodiscard]] T* operator->() {
        _::entity_archetype& entity_archetype = _core->entity_archetypes[_::entity_id_index(_entity_id)];
        _::archetype& archetype = _core->archetypes[_::archetype_id_index(entity_archetype.archetype)];
        if (_::archetype_mask_has_component(archetype.mask, _id))
            return (T*) _core->entity_archetype_component(entity_archetype, _id);
        else throw std::runtime_error("Component does not exist");
    }

    [[nodiscard]] T& operator*() {
        _::entity_archetype& entity_archetype = _core->entity_archetypes[_::entity_id_index(_entity_id)];
        _::archetype& archetype = _core->archetypes[_::archetype_id_index(entity_archetype.archetype)];
        if (_::archetype_mask_has_component(archetype.mask, _id))
            return *(T*) _core->entity_archetype_component(entity_archetype, _id);
        else throw std::runtime_error("Component does not exist");
    }
};

class world;
class entity {
    friend class world;

    entity_id _id;
    _::ecs_data* _core;

    entity(entity_id id, _::ecs_data* core) : _id(id), _core(core) { }

  public:
    [[nodiscard]] entity_id id() const {
        return _id;
    }
    [[nodiscard]] bool alive() const {
        return _::entity_id_index(_id) < _core->entities.size() && _core->entities[_::entity_id_index(_id)] == _id;
    }
    [[nodiscard]] bool dead() const {
        return !alive();
    }

    template <typename T>
    [[nodiscard]] result::val<component<T>> get() const {
        if (!alive()) return result::err("Entity is dead");
        component_id component_id = _core->lookup_component_id<T>();
        _::entity_archetype& entity_archetype = _core->entity_archetypes[_::entity_id_index(_id)];
        _::archetype& archetype = _core->archetypes[_::archetype_id_index(entity_archetype.archetype)];
        if (!_::archetype_mask_has_component(archetype.mask, component_id))
            return result::err("Component does not exist");

        return result::ok(component<T>(component_id, _id, _core));
    }

    template <typename T>
    [[nodiscard]] bool has() const {
        if (!alive()) return false;
        component_id component_id = _core->lookup_component_id<T>();
        _::entity_archetype& entity_archetype = _core->entity_archetypes[_::entity_id_index(_id)];
        _::archetype& archetype = _core->archetypes[_::archetype_id_index(entity_archetype.archetype)];
        return _::archetype_mask_has_component(archetype.mask, component_id);
    }

    template <typename T, typename... Args>
    result::val<component<T>> add(Args&&... args) {
        if (!alive()) return result::err("Entity is dead");

        _::entity_archetype& entity_archetype = _core->entity_archetypes[_::entity_id_index(_id)];
        _::archetype& old_archetype = _core->archetypes[_::archetype_id_index(entity_archetype.archetype)];

        component_id component_id = _core->lookup_component_id<T>();
        archetype_mask new_mask = _::archetype_mask_add_component(old_archetype.mask, component_id);
        if (old_archetype.mask == new_mask) return result::err("Component already exists");

        _core->move_entity_to_archetype(_id, _core->get_or_create_archetype(new_mask));
        T* component_ptr = (T*) _core->entity_archetype_component(entity_archetype, component_id);
        new (component_ptr) T(std::forward<Args>(args)...);
        return result::ok(component<T>(component_id, _id, _core));
    }

    template <typename T>
    result::val<component<T>> set(const T& component) {
        if (!alive()) return result::err("Entity is dead");

        _::entity_archetype& entity_archetype = _core->entity_archetypes[_::entity_id_index(_id)];
        _::archetype& old_archetype = _core->archetypes[_::archetype_id_index(entity_archetype.archetype)];

        component_id component_id = _core->lookup_component_id<T>();
        archetype_mask new_mask = _::archetype_mask_add_component(old_archetype.mask, component_id);
        if (old_archetype.mask != new_mask)
            _core->move_entity_to_archetype(_id, _core->get_or_create_archetype(new_mask));

        T* component_ptr = (T*) _core->entity_archetype_component(entity_archetype, component_id);
        std::memcpy(component_ptr, &component, sizeof(T));
        return result::ok(::saturn::component<T>(component_id, _id, _core));
    }

    template <typename T>
    void remove() {
        if (!alive()) return;

        _::entity_archetype& entity_archetype = _core->entity_archetypes[_::entity_id_index(_id)];
        _::archetype& old_archetype = _core->archetypes[_::archetype_id_index(entity_archetype.archetype)];

        component_id component_id = _core->lookup_component_id<T>();
        archetype_mask new_mask = _::archetype_mask_remove_component(old_archetype.mask, component_id);
        if (old_archetype.mask == new_mask) return;

        _core->move_entity_to_archetype(_id, _core->get_or_create_archetype(new_mask));
    }
};

class universe;
class world {
    friend class universe;

    std::unique_ptr<_::ecs_data> _core;

    world() : _core(std::make_unique<_::ecs_data>()) { }

  public:
    ~world() = default;
    world(const world&) = delete;

    [[nodiscard]] result::val<entity> create_entity() {
        _::archetype& empty_archetype = _core->archetypes[_core->empty_archetype_index];
        if (_core->free_entities.empty()) {
            entity_id id = _::create_entity_id(_core->entities.size(), 0);
            _core->entities.push_back(id);
            _core->entity_archetypes.push_back({empty_archetype.id, 0});
            _core->add_entity_to_archetype(_core->entities.back(), empty_archetype);
            return result::ok(entity(id, _core.get()));
        } else {
            array_index index = _core->free_entities.back();
            entity_id id = _core->entities[index];
            _core->free_entities.pop_back();
            _core->add_entity_to_archetype(id, empty_archetype);
            return result::ok(entity(id, _core.get()));
        }
    }

    void destroy_entity(class entity entity) {
        if (!entity.alive()) return;
        _core->remove_entity_from_archetype(entity._id);
        _core->free_entities.push_back(_::entity_id_index(entity._id));
        entity_id new_id = _::create_entity_id(_::entity_id_index(entity._id), _::entity_id_version(entity._id) + 1);
        _core->entities[_::entity_id_index(entity._id)] = new_id;
    }
};

class universe {
  public:
    universe() = default;
    universe(const universe&) = delete;
    static result::ptr<universe> create() {
        return result::ok(new universe());
    }
    result::ptr<world> create_world() {
        return result::ok(new world());
    }
};

} // namespace saturn

#endif
