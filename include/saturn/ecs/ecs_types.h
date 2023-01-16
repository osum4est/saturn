#ifndef SATURN_ECS_TYPES_H
#define SATURN_ECS_TYPES_H

#include <ratio>
#include <cstdint>

namespace saturn {

class universe;
class world;
class entity;
template <typename T>
class component;
template <typename... T>
class query_iterator;
template <typename... T>
class query;

typedef float delta_time;
typedef std::ratio<1, 1> delta_time_period;

typedef uint32_t array_index;
typedef uint64_t entity_id;
typedef uint16_t component_id;
typedef uint32_t stage_id;
typedef uint32_t system_id;

const entity_id INVALID_ENTITY_ID = -1;

#define SATURN_ECS_MAX_COMPONENTS 64
typedef uint64_t archetype_mask;
typedef uint32_t archetype_id;
typedef uint64_t archetype_component_id;

} // namespace saturn

#endif
