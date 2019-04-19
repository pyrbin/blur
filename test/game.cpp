
#include <iostream>
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

    world.mod_comp<Velocity>(ent, [](auto& vel) {
        // Set default velocity force value
        vel.f += 5;
    });

    // TODO: implement modify multiple
    // world.mod_comp<Velocity, Position>(
    //    ent, [](Velocity vel, Position pos) { pos.x += vel.f; });

    // Test .has_comp
    print_nice("Test .has_comp");
    const auto has_pos = world.has_comp<Position>(ent);
    const auto has_pos_n_vel = world.has_comp<Position, Velocity>(ent);
    const auto has_player = world.has_comp<Player>(ent);
    std::cout << "Has Pos: " << (has_pos ? "true" : "false") << "\n";
    std::cout << "Has Pos&Vel: " << (has_pos_n_vel ? "true" : "false") << "\n";
    std::cout << "Has Player: " << (has_player ? "true" : "false") << "\n";

    // Test .del_comp
    // world.del_comp<Position>(ent);

    // Test .add_comp
    // world.add_comp<Position>(ent);

    // Add a test system
    world.insert<TestSystem>();
    world.insert<OnlyPlayerSystem>();

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
