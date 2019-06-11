
#include <functional>
#include <iostream>
#include <memory>
#include <string>

#include "../src/blur.hpp"

#include "data.hpp"

void print_nice(std::string msg) {
    std::cout << "==================\n"
              << msg << "\n"
              << "==================\n";
}

int main() {
    // Create a world
    auto world = blur::World();

    // Create an archetype
    auto aty = blur::Archetype::of<Velocity, Position>();

    // Create an entitys
    // which allocates a new archetype block
    print_nice("Added Vel entity");
    auto ent = world.create(aty);

    // Modify multiple

    world.add_comp<Player>(ent);

    auto en4 = world.create(aty);
    auto en42 = world.create(aty);

    world.mod_comp(ent, [](Velocity& vel) { vel.f = 5; });

    world.insert<MovSystem>();

    world.tick();
}
