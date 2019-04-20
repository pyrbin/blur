
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
    auto aty = blur::Archetype<Position, Velocity>();

    // Create an entitys
    // which allocates a new archetype block
    print_nice("Added Pos & Vel entity");
    auto ent = world.create(aty);

    // Modify multiple
    // world.mod_comp<Velocity>(ent, [](Velocity& vel) { vel.f += 5; });
    world.mod_comp(ent, [](Velocity& vel) {
        // mods velo
        vel.f += 5;
    });

    world.mod_comp(ent, [](const Velocity& vel, Position& pos) {
        // mods velo
        pos.y += vel.f;
    });

    // Test .has_comp
    print_nice("Test .has_comp");
    const auto has_pos = world.has_comp<Position>(ent);
    const auto has_pos_n_vel = world.has_comp<Position, Velocity>(ent);
    const auto has_player = world.has_comp<Player>(ent);
    std::cout << "has Pos: " << (has_pos ? "true" : "false") << "\n";
    std::cout << "has Pos & Vel: " << (has_pos_n_vel ? "true" : "false") << "\n";
    std::cout << "has Player: " << (has_player ? "true" : "false") << "\n";

    // Test .del_comp
    // world.del_comp<Position>(ent);

    // Test .add_comp
    // world.add_comp<Position>(ent);

    // Add a test system
    world.insert<MovSystem>();
    world.insert<PlayerSystem>();

    // Simulate 3 ticks
    print_nice("Simulate 3 ticks");
    for (unsigned i{0}; i < 3; i++) {
        world.tick();
    }

    // Create an entitys
    // which allocates a new archetype block
    auto pla = world.create<Player>();
    print_nice("Added a player");

    // Simulate 3 ticks
    print_nice("Simulate 3 ticks");
    for (unsigned i{0}; i < 3; i++) {
        world.tick();
    }
}
