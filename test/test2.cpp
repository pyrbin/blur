
#include <iostream>

#include "../src/blur.hpp"
#include "tmp.hpp"

int main() {
    auto world = blur::World();
    auto ent = world.create<Position, Velocity>();
    auto& vel = world.get_comp<Velocity>(ent);
    vel.f = 5;
    world.insert<TestSystem>();
    world.update();
}
