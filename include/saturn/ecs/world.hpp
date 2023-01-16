#ifndef SATURN_WORLD_HPP
#define SATURN_WORLD_HPP

#include "ecs_core.hpp"
#include "entity.hpp"
#include "query.hpp"
#include "stage.h"
#include "system.hpp"
#include "trait_helpers.h"
#include <unordered_set>

namespace saturn {

class world {
    friend class universe;

    std::unique_ptr<_::ecs_core> _core;

    // Stages
    std::unordered_map<system_id, std::unique_ptr<_::system_base>> _systems = {};
    std::unordered_map<stage_id, std::unordered_set<system_id>> _systems_by_stage = {};
    // TODO: Separate map for custom stages

    delta_time _update_dt = 0;
    std::chrono::high_resolution_clock::time_point _last_update_time = std::chrono::high_resolution_clock::now();
    std::chrono::high_resolution_clock::time_point _current_update_time = std::chrono::high_resolution_clock::now();

  public:
    world() : _core(std::make_unique<_::ecs_core>()) {
        _systems_by_stage[stages::pre_update] = {};
        _systems_by_stage[stages::update] = {};
        _systems_by_stage[stages::post_update] = {};
    }

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
        return create_query_from_type<query<T...>>();
    }

    template <typename... T, typename F>
    system_id create_system(stage stage, F&& func) {
        std::unique_ptr<_::system_base> system;

        if constexpr (std::is_assignable_v<system_func_with_context<T...>, F>) {
            system = std::make_unique<_::func_system_with_ctx<T...>>(create_query<T...>(), func);
        } else if constexpr (std::is_assignable_v<system_func<T...>, F>) {
            system = std::make_unique<_::func_system<T...>>(create_query<T...>(), func);
        } else {
            static_assert(always_false_v<decltype(func)>, "Invalid system function signature");
        }

        return add_system(stage, std::move(system));
    }

    template <typename... T, typename F>
    system_id create_system(F&& func) {
        return create_system<T...>(stages::update, func);
    }

    template <typename S>
    system_id create_system(stage stage = stages::update) {
        using struct_pointer_system_t = typename S::template forward_args_t<_::struct_ptr_system>;
        auto query = create_query_from_type<typename S::query_t>();
        return add_system(stage, std::make_unique<struct_pointer_system_t>(query, std::make_unique<S>()));
    }

    void destroy_system(system_id system) {
        _systems.erase(system);
        for (auto& [stage, systems] : _systems_by_stage) {
            systems.erase(system);
        }
    }

    void update() {
        _current_update_time = std::chrono::high_resolution_clock::now();
        _update_dt = std::chrono::duration_cast<std::chrono::duration<delta_time, delta_time_period>>(
                         _current_update_time - _last_update_time)
                         .count();
        _last_update_time = _current_update_time;

        update_stage(stages::pre_update);
        update_stage(stages::update);
        update_stage(stages::post_update);
    }

  private:
    // TODO: Not sure how to improve this
    template <typename T>
    T create_query_from_type() {
        return T(_core.get());
    }

    system_id add_system(stage stage, std::unique_ptr<_::system_base> system) {
        system_id id = _core->create_system_id();
        _systems[id] = std::move(system);
        _systems_by_stage[stage].insert(id);
        return id;
    }

    void update_stage(stage stage) {
        system_context ctx(_core.get(), _update_dt);
        for (system_id id : _systems_by_stage[stage])
            _systems[id]->run(ctx);
    }
};

} // namespace saturn

#endif
