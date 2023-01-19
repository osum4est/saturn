#include <saturn/saturn.h>

int main() {
    auto window = saturn::window::create({1280, 720, "Saturn Simple Example"}).get();
    auto universe = saturn::universe::create();
    auto world = universe->create_world();

    while (!window->should_close()) {
        window->poll_events();
        world->update();
    }
}