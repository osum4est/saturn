#ifndef SATURN_ECS_CORE_HPP
#define SATURN_ECS_CORE_HPP

#include "ecs_types.h"
#include <cstdlib>
#include <cstring>
#include <result/result.h>
#include <saturn/ecs/utils/component_pool.hpp>

// Hide this stuff from the user, they shouldn't need to use it
namespace saturn::_ {

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

} // namespace saturn::_

#endif
