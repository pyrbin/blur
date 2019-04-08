# WIP tiny ECS written in C++17

## Motivation

Goal of this project is to create a simple ECS game engine to increase my knowledge of modern C++, DoD pattern and general game/game engine development.
The main focus will be to create the "ECS-part" and then later add stuff like netcode & rendering etc.

## Design goals

The main design goals/rules:

- Components have no functions/logic (other than occasional helpers/getters)
- Systems have no state and are generally one-function objects
- All shared code will live inside Utils.
- Systems cant call other systems (No communication)
- Components & entities will be stored by archetypes (every unique set of components will become a unique archetypes)
- Components can choose to opt out (wont affect archetype, eg. if X = not monitored set(X,Y,Z) will be same as set(Y,Z))

These goals have been motivated through various youtube talks and blog posts.

## Proposed API
### Entities & Components
```cpp
struct Position {
    float x;
    float y;
}

auto entity = world.create<Position>()
auto arch = Archetype<Position>();
auto alt = world.create(arch);

```
### Systems
```cpp
struct TestSystem {
    void Execute(Position& pos, Velocity& vel..., Entity en) {
        // do system logic
    }
}
```
