#ifndef SATURN_ECS_HPP
#define SATURN_ECS_HPP

#include <cstdlib>
#include <cstring>
#include <result/result.h>

namespace saturn {

class entity;
class world;
template <typename T>
class component;
template <typename... T>
class query_iterator;

typedef uint32_t array_index;
typedef uint32_t world_id;
typedef uint64_t entity_id;
typedef uint16_t component_id;

const entity_id INVALID_ENTITY_ID = -1;

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

bool archetype_mask_matches(archetype_mask mask, archetype_mask other);
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

    void push_back() {
        if (_component_count >= _capacity) resize(_capacity * 2);
        _component_count++;
    }

    void* operator[](array_index index) const {
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
    std::vector<entity_id> entities;
    std::vector<component_pool> component_pools;
};

struct entity_archetype {
    array_index archetype_index;
    array_index archetype_entity_index;
};

struct ecs_core {
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

    ecs_core() {
        const auto empty_archetype_mask = 0;
        auto& empty_archetype = get_or_create_archetype(empty_archetype_mask);
        empty_archetype_id = empty_archetype.id;
        empty_archetype_index = archetype_id_index(empty_archetype_id);
    }

    static component_id _next_component_id;
    template <typename T>
    std::enable_if_t<!std::is_const_v<T>, component_id> lookup_component_id() {
        return lookup_component_id<const T>();
    }

    template <typename T>
    std::enable_if_t<std::is_const_v<T>, component_id> lookup_component_id() {
        static component_id id = 0;
        static bool initialized = false;
        if (initialized) return id;
        else {
            initialized = true;
            component_sizes.push_back(sizeof(T));
            return id = _next_component_id++;
        }
    }

    template <typename... T>
    archetype_mask create_archetype_mask() {
        archetype_mask mask = 0;
        (..., (mask = archetype_mask_add_component(mask, lookup_component_id<T>())));
        return mask;
    }

    bool entity_alive(entity_id id) {
        auto index = entity_id_index(id);
        return index < entities.size() && entities[index] == id;
    }

    archetype& get_or_create_archetype(archetype_mask mask) {
        auto it = archetypes_by_mask.find(mask);
        if (it != archetypes_by_mask.end()) return archetypes[it->second];

        archetype_id id = create_archetype_id(archetypes.size());
        archetypes.push_back(archetype {.id = id, .mask = mask, .entities = {}, .component_pools = {}});
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
        archetype& archetype = archetypes[entity_archetype.archetype_index];
        if (archetype.component_pools.empty()) return nullptr;

        archetype_component_id archetype_component_id = create_archetype_component_id(archetype.id, component);
        auto it = archetype_component_indices.find(archetype_component_id);
        if (it == archetype_component_indices.end()) return nullptr;
        // TODO: Could do 2D array instead of map
        return archetype.component_pools[it->second][entity_archetype.archetype_entity_index];
    }

    void move_entity_to_archetype(entity_id entity, archetype& archetype) {
        entity_archetype& entity_archetype = entity_archetypes[entity_id_index(entity)];
        struct archetype& old_archetype = archetypes[entity_archetype.archetype_index];
        if (old_archetype.id == archetype.id) return;

        array_index old_archetype_index = entity_archetype.archetype_entity_index;
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
        archetype& archetype = archetypes[entity_archetype.archetype_index];

        archetype_free_entities[archetype_id_index(archetype.id)].push_back(entity_archetype.archetype_entity_index);
        archetype.entities[entity_archetype.archetype_entity_index] = INVALID_ENTITY_ID;
        entity_archetype.archetype_index = -1;
        entity_archetype.archetype_entity_index = -1;
    }

    void add_entity_to_archetype(entity_id entity, archetype& archetype) {
        entity_archetype& entity_archetype = entity_archetypes[entity_id_index(entity)];
        entity_archetype.archetype_index = archetype_id_index(archetype.id);

        auto& free_archetype_entities = archetype_free_entities[archetype_id_index(archetype.id)];
        if (free_archetype_entities.empty()) {
            entity_archetype.archetype_entity_index = archetype.entities.size();
            archetype.entities.push_back(entity);
            for (auto& pool : archetype.component_pools)
                pool.push_back();
        } else {
            entity_archetype.archetype_entity_index = free_archetype_entities.back();
            archetype.entities[entity_archetype.archetype_entity_index] = entity;
            free_archetype_entities.pop_back();
        }
    }
};
} // namespace _

template <typename T>
class component {
    friend class entity;

    component_id _id;
    entity_id _entity_id;
    _::ecs_core* _core;

    component(component_id id, entity_id entity_id, _::ecs_core* core) : _id(id), _entity_id(entity_id), _core(core) { }

    [[nodiscard]] bool valid() const {
        return _core != nullptr && _entity_id != INVALID_ENTITY_ID && _core->entity_alive(_entity_id);
    }

  public:
    [[nodiscard]] bool operator==(const component& other) const {
        return _id == other._id && _entity_id == other._entity_id && _core == other._core;
    }

    [[nodiscard]] bool operator!=(const component& other) const {
        return !(*this == other);
    }

    [[nodiscard]] T* operator->() {
        if (!valid()) throw std::runtime_error("Entity is dead");

        _::entity_archetype& entity_archetype = _core->entity_archetypes[_::entity_id_index(_entity_id)];
        _::archetype& archetype = _core->archetypes[entity_archetype.archetype_index];
        if (_::archetype_mask_has_component(archetype.mask, _id))
            return (T*) _core->entity_archetype_component(entity_archetype, _id);
        else throw std::runtime_error("Component does not exist");
    }

    [[nodiscard]] T& operator*() {
        if (!valid()) throw std::runtime_error("Entity is dead");

        _::entity_archetype& entity_archetype = _core->entity_archetypes[_::entity_id_index(_entity_id)];
        _::archetype& archetype = _core->archetypes[entity_archetype.archetype_index];
        if (_::archetype_mask_has_component(archetype.mask, _id))
            return *(T*) _core->entity_archetype_component(entity_archetype, _id);
        else throw std::runtime_error("Component does not exist");
    }
};

class entity {
    friend class world;
    template <typename... T>
    friend class query_iterator;

    entity_id _id;
    _::ecs_core* _core;

    entity(entity_id id, _::ecs_core* core) : _id(id), _core(core) { }

  public:
    entity() : _id(INVALID_ENTITY_ID), _core(nullptr) { }

    [[nodiscard]] entity_id id() const {
        return _id;
    }

    [[nodiscard]] bool alive() const {
        return _core != nullptr && _id != INVALID_ENTITY_ID && _core->entity_alive(_id);
    }

    [[nodiscard]] bool dead() const {
        return !alive();
    }

    bool operator==(const entity& other) const {
        return _id == other._id;
    }

    bool operator!=(const entity& other) const {
        return _id != other._id;
    }

    // TODO: Return invalid component instead of result?
    template <typename T>
    [[nodiscard]] result::val<component<T>> get() const {
        if (!alive()) return result::err("Entity is dead");
        component_id component_id = _core->lookup_component_id<T>();
        _::entity_archetype& entity_archetype = _core->entity_archetypes[_::entity_id_index(_id)];
        _::archetype& archetype = _core->archetypes[entity_archetype.archetype_index];
        if (!_::archetype_mask_has_component(archetype.mask, component_id))
            return result::err("Component does not exist");

        return result::ok(component<T>(component_id, _id, _core));
    }

    template <typename T>
    [[nodiscard]] bool has() const {
        if (!alive()) return false;
        component_id component_id = _core->lookup_component_id<T>();
        _::entity_archetype& entity_archetype = _core->entity_archetypes[_::entity_id_index(_id)];
        _::archetype& archetype = _core->archetypes[entity_archetype.archetype_index];
        return _::archetype_mask_has_component(archetype.mask, component_id);
    }

    // TODO: Support const components?
    template <typename T, typename... Args>
    std::enable_if_t<!std::is_const_v<T>, result::val<component<T>>> add(Args&&... args) {
        if (!alive()) return result::err("Entity is dead");

        _::entity_archetype& entity_archetype = _core->entity_archetypes[_::entity_id_index(_id)];
        _::archetype& old_archetype = _core->archetypes[entity_archetype.archetype_index];

        component_id component_id = _core->lookup_component_id<T>();
        archetype_mask new_mask = _::archetype_mask_add_component(old_archetype.mask, component_id);
        if (old_archetype.mask == new_mask) return result::err("Component already exists");

        _core->move_entity_to_archetype(_id, _core->get_or_create_archetype(new_mask));
        T* component_ptr = (T*) _core->entity_archetype_component(entity_archetype, component_id);
        new (component_ptr) T(std::forward<Args>(args)...);
        return result::ok(component<T>(component_id, _id, _core));
    }

    template <typename T>
    std::enable_if_t<!std::is_const_v<T>, result::val<component<T>>> set(const T& component) {
        if (!alive()) return result::err("Entity is dead");

        _::entity_archetype& entity_archetype = _core->entity_archetypes[_::entity_id_index(_id)];
        _::archetype& old_archetype = _core->archetypes[entity_archetype.archetype_index];

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
        _::archetype& old_archetype = _core->archetypes[entity_archetype.archetype_index];

        component_id component_id = _core->lookup_component_id<T>();
        archetype_mask new_mask = _::archetype_mask_remove_component(old_archetype.mask, component_id);
        if (old_archetype.mask == new_mask) return;

        _core->move_entity_to_archetype(_id, _core->get_or_create_archetype(new_mask));
    }
};

template <typename... T>
class query_iterator {
    using iterator_concept [[maybe_unused]] = std::forward_iterator_tag;
    using value_type = std::tuple<const entity, T&...>;

    _::ecs_core* _core;
    archetype_mask _mask;

    array_index _current_archetype_index;
    array_index _current_entity_index;

    array_index _component_indices[sizeof...(T)];

    query_iterator(_::ecs_core* core, archetype_mask mask, size_t archetype_index, size_t entity_index)
        : _core(core),
          _mask(mask),
          _current_archetype_index(archetype_index),
          _current_entity_index(entity_index),
          _component_indices() { }

    template <size_t... I>
    void advance_to_next_archetype(std::index_sequence<I...>) {
        while (_current_archetype_index == -1 || _current_archetype_index < _core->archetypes.size()) {
            _current_archetype_index++;
            _current_entity_index = -1;

            if (_current_archetype_index >= _core->archetypes.size()) return;

            const auto& current_archetype = _core->archetypes[_current_archetype_index];
            if (_::archetype_mask_matches(current_archetype.mask, _mask)) {
                auto entity_count = current_archetype.entities.size();

                // Skip this archetype if it has no entities
                if (!entity_count) continue;

                // Skip this archetype if all it's entities are dead
                if (_core->archetype_free_entities[_current_archetype_index].size() == entity_count) continue;

                ((_component_indices[I] = _core->archetype_component_indices[_::create_archetype_component_id(
                      current_archetype.id, _core->lookup_component_id<T>())]),
                 ...);
                return;
            }
        }
    }

    void advance_to_next_alive_entity() {
        while (_current_archetype_index < _core->archetypes.size()) {
            _current_entity_index++;

            const auto& current_archetype = _core->archetypes[_current_archetype_index];
            if (_current_entity_index >= current_archetype.entities.size()) {
                advance_to_next_archetype(std::index_sequence_for<T...>());
                continue;
            }

            entity_id id = current_archetype.entities[_current_entity_index];
            if (_core->entity_alive(id)) return;
        }
    }

    template <size_t... I>
    value_type current_result(std::index_sequence<I...>) {
        const auto& current_archetype = _core->archetypes[_current_archetype_index];
        return std::tuple<const entity, T&...>(
            entity(current_archetype.entities[_current_entity_index], _core),
            *(T*) current_archetype.component_pools[_component_indices[I]][_current_entity_index]...);
    }

  public:
    static query_iterator begin(_::ecs_core* core, archetype_mask mask) {
        auto it = query_iterator(core, mask, -1, -1);
        it.advance_to_next_archetype(std::index_sequence_for<T...>());
        it.advance_to_next_alive_entity();
        return it;
    }

    static query_iterator end(_::ecs_core* core, archetype_mask mask) {
        return query_iterator(core, mask, core->archetypes.size(), -1);
    }

    query_iterator& operator++() {
        advance_to_next_alive_entity();
        return *this;
    }

    query_iterator operator++(int) const {
        query_iterator tmp = *this;
        operator++();
        return tmp;
    }

    bool operator==(const query_iterator& other) const {
        return _core == other._core && _mask == other._mask &&
               _current_archetype_index == other._current_archetype_index &&
               _current_entity_index == other._current_entity_index;
    }

    bool operator!=(const query_iterator& other) const {
        return !(*this == other);
    }

    value_type operator*() {
        return current_result(std::index_sequence_for<T...>());
    }
};

template <typename... T>
class query {
    friend class world;

    using type_indices = std::make_index_sequence<sizeof...(T)>;

    _::ecs_core* _core;
    archetype_mask _mask;

    query(_::ecs_core* core) : _core(core), _mask(_core->create_archetype_mask<T...>()) { }

  public:
    typedef query_iterator<T...> iterator;

    iterator begin() {
        return iterator::begin(_core, _mask);
    }

    iterator end() {
        return iterator::end(_core, _mask);
    }

    size_t count() {
        size_t count = 0;
        for (const auto& result : *this)
            count++;
        return count;
    }
};

class universe;
class world {
    friend class universe;

    std::unique_ptr<_::ecs_core> _core;

    world() : _core(std::make_unique<_::ecs_core>()) { }

  public:
    ~world() = default;
    world(const world&) = delete;

    [[nodiscard]] entity create_entity() {
        _::archetype& empty_archetype = _core->archetypes[_core->empty_archetype_index];
        if (_core->free_entities.empty()) {
            entity_id id = _::create_entity_id(_core->entities.size(), 0);
            _core->entities.push_back(id);
            _core->entity_archetypes.push_back({empty_archetype.id, 0});
            _core->add_entity_to_archetype(_core->entities.back(), empty_archetype);
            return {id, _core.get()};
        } else {
            array_index index = _core->free_entities.back();
            entity_id id = _core->entities[index];
            _core->free_entities.pop_back();
            _core->add_entity_to_archetype(id, empty_archetype);
            return {id, _core.get()};
        }
    }

    void destroy_entity(class entity entity) {
        if (!entity.alive()) return;
        _core->remove_entity_from_archetype(entity._id);
        _core->free_entities.push_back(_::entity_id_index(entity._id));
        entity_id new_id = _::create_entity_id(_::entity_id_index(entity._id), _::entity_id_version(entity._id) + 1);
        _core->entities[_::entity_id_index(entity._id)] = new_id;
    }

    template <typename... T>
    [[nodiscard]] query<T...> create_query() {
        return query<T...>(_core.get());
    }
};

class universe {
    universe() = default;

  public:
    universe(const universe&) = delete;
    static std::unique_ptr<universe> create() {
        return std::unique_ptr<universe>(new universe());
    }
    std::unique_ptr<world> create_world() {
        return std::unique_ptr<world>(new world());
    }
};

} // namespace saturn

#endif
