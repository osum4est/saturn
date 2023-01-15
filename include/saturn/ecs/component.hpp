#ifndef SATURN_COMPONENT_HPP
#define SATURN_COMPONENT_HPP

#include "ecs_core.hpp"

namespace saturn {

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

} // namespace saturn

#endif
