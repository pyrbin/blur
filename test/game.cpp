
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <chrono>

#include "../src/blur.hpp"

#include "data.hpp"

using namespace blur;

int main() {
    const unsigned ENTS = 10;
    const unsigned ITER = 1;

    float tot_duration{0};


    for (auto t{0}; t < ITER; t++) {
        // Create new world
        auto world = World{};

        // Create an archetype
        auto arch = Archetype::of<Position, Velocity>();

        world.insert<MoveSystem>();
        
        auto start = std::chrono::high_resolution_clock::now(); 
        // Create an entity
        auto entt = world.batch(ENTS);
        for(int i{0}; i < 10000; i++) {
            world.tick();
        }
        world.remove(entt);

        auto finish = std::chrono::high_resolution_clock::now(); 

        tot_duration += std::chrono::duration<float>(finish - start).count();

    }
    std::cout << "Batch size: " << ENTS << "\n";
    std::cout << "Took: " << (float)((float)tot_duration/(float)ITER) << " seconds. \n";

}

