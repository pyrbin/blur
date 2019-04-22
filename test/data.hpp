#include <iostream>

struct Position {
    float x{0};
    float y{0};
};

struct Velocity {
    float f{0};
};

struct Player {
    /* data */
};

struct MovSystem {
    void process(Position& pos, Velocity& vel) const {
        std::cout << "Position [" << pos.x << ", " << pos.y << "]\n";
        pos.x += vel.f;
        pos.y -= vel.f;
    }
};

struct PlayerSystem {
    void process(Player& pl) const {
        std::cout << "Process players!"
                  << "\n";
    }
};