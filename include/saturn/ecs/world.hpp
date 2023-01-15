#ifndef SATURN_WORLD_HPP
#define SATURN_WORLD_HPP

#include "ecs_core.hpp"
#include "entity.hpp"
#include "query.hpp"

namespace saturn {

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

} // namespace saturn

#endif
