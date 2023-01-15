#ifndef SATURN_COMPONENT_POOL_HPP
#define SATURN_COMPONENT_POOL_HPP

#include "../ecs_types.h"

// TODO: Clean these up when the world is destroyed
namespace saturn::_ {

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

} // namespace saturn::_

#endif
