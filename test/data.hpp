#include <iomanip>
#include <iostream>

struct Position {
    float x{0}, y{0};
};

struct Velocity {
    float f{1};
};

struct Sprite {
    char symbol = '?';
};

struct MoveSystem {
    void process(Velocity& vel, Position& pos) const {
        pos.x += vel.f + 1;
    }
};