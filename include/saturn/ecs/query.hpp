#ifndef SATURN_QUERY_HPP
#define SATURN_QUERY_HPP

#include "ecs_core.hpp"

namespace saturn {

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

    _::ecs_core* _core;
    archetype_mask _mask;

    explicit query(_::ecs_core* core) : _core(core), _mask(_core->create_archetype_mask<T...>()) { }

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

} // namespace saturn

#endif
