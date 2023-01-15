#ifndef SATURN_ENTITY_HPP
#define SATURN_ENTITY_HPP

#include "ecs_core.hpp"
#include "component.hpp"

namespace saturn {

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
    std::enable_if_t<!std::is_const_v<T>, result::val<component<T>>> set(T&& component) {
        if (!alive()) return result::err("Entity is dead");

        _::entity_archetype& entity_archetype = _core->entity_archetypes[_::entity_id_index(_id)];
        _::archetype& old_archetype = _core->archetypes[entity_archetype.archetype_index];

        component_id component_id = _core->lookup_component_id<T>();
        archetype_mask new_mask = _::archetype_mask_add_component(old_archetype.mask, component_id);
        if (old_archetype.mask != new_mask) {
            _core->move_entity_to_archetype(_id, _core->get_or_create_archetype(new_mask));
            T* component_ptr = (T*) _core->entity_archetype_component(entity_archetype, component_id);
            new (component_ptr) T(std::forward<T>(component));
        } else {
            T* component_ptr = (T*) _core->entity_archetype_component(entity_archetype, component_id);
            *component_ptr = std::forward<T>(component);
        }

        return result::ok(::saturn::component<T>(component_id, _id, _core));
    }

    template <typename T>
    std::enable_if_t<!std::is_const_v<T>, result::val<component<T>>> set(const T& component) {
        return set<T>(T(component));
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

}

#endif
