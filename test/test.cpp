#include <time.h>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "../src/blur.hpp"

#define TICK_COUNT 10
#define TICK_MS 100

// Components
struct Pos {
    float x{0};
    float y{0};
};

struct Vel {
    float force{0};
};

struct Sprite {
    size_t symbol{0};
};

// Test "systems"
struct MovSystem {
    void update(Pos& p, Vel& v) const {
        // simple acceleration
        p.x += v.force;
        v.force = ((rand() % 9) + 1);
    }
};

const char resource[] = "abcdefghijklmnopqrstuvwxyz";
void render(blur::World& w, std::vector<blur::Entity> en) {
    std::cout << " ====== RENDER FRAME ====== "
              << "\n";
    for (auto& e : en) {
        auto& pos = w.get_comp<Pos>(e);
        auto& spr = w.get_comp<Sprite>(e);
        std::cout << std::setw(pos.x) << resource[spr.symbol] << "\n";
    }
}

// Test components
int main() {
    // ------ Init
    srand(time(NULL));
    std::vector<blur::Entity> entities;
    auto world = blur::World();
    auto movsys = MovSystem();
    for (size_t i{1}; i < 6; i++) {
        auto e = world.create<Pos, Vel, Sprite>();
        entities.push_back(e);
        auto& vel = world.get_comp<Vel>(e);
        auto& spr = world.get_comp<Sprite>(e);
        vel.force = ((rand() % 9) + 1);
        spr.symbol = i - 1;
    }
    // ------ Update (simulate ticks)
    for (size_t i{0}; i < TICK_COUNT; i++) {
        blur::EntityId j{0};
        for (auto en : entities) {
            blur::Entity e = {j, 0};
            auto& vel = world.get_comp<Vel>(e);
            auto& pos = world.get_comp<Pos>(e);
            movsys.update(pos, vel);
            j++;
        }
        // ------ Render (simulate 64 ticks)
        render(world, entities);
        std::this_thread::sleep_for(std::chrono::milliseconds(TICK_MS));
    }
}