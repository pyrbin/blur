#include <iostream>

struct Position {
    float x;
    float y;
};

struct Velocity {
    float f;
};

struct Player {
    /* data */
};

struct MovSystem {
    void process(Position& pos, Velocity& vel) const {
        std::cout << "Position [" << pos.x << ", " << pos.y << "]\n";
        pos.x += vel.f;
        pos.y += vel.f;
    }
};

struct PlayerSystem {
    void process(Player& pl) const {
        std::cout << "Process players!"
                  << "\n";
    }
};