#ifndef SATURN_UNIVERSE_HPP
#define SATURN_UNIVERSE_HPP

#include "ecs_core.hpp"
#include "world.hpp"

namespace saturn {

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
