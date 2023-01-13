#include "saturn/ecs/ecs.hpp"

namespace saturn::_ {

array_index entity_id_index(entity_id id) {
    return id & 0xFFFFFFFF;
}

uint32_t entity_id_version(entity_id id) {
    return id >> 32;
}

entity_id create_entity_id(array_index index, uint32_t version) {
    return (entity_id) version << 32 | index;
}

uint32_t archetype_id_index(archetype_id id) {
    return id & 0xFFFFFFFF;
}

archetype_id create_archetype_id(array_index index) {
    return (archetype_id) index;
}

archetype_component_id create_archetype_component_id(archetype_id archetype, component_id component) {
    return (archetype_component_id) archetype | ((archetype_component_id) component << 32);
}

array_index component_id_bit_index(component_id id) {
    return id & 0xFFFF;
}

component_id create_component_id(array_index bit_index) {
    return (component_id) bit_index;
}

bool archetype_mask_has_component(archetype_mask mask, component_id component) {
    return mask & ((archetype_mask) 1 << component_id_bit_index(component));
}

archetype_mask archetype_mask_add_component(archetype_mask mask, component_id component) {
    return mask | ((archetype_mask) 1 << component_id_bit_index(component));
}

archetype_mask archetype_mask_remove_component(archetype_mask mask, component_id component) {
    return mask & ~((archetype_mask) 1 << component_id_bit_index(component));
}

std::vector<size_t> ecs_data::component_sizes = {};
component_id ecs_data::_next_component_id = 0;

} // namespace saturn::_