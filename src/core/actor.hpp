#pragma once

#include <memory>

#include "entity.hpp"
#include "world.hpp"

namespace blur {

struct Actor {
    using world_ptr = std::shared_ptr<World>;
    
    Entity entity;
    world_ptr world;

    Actor(Entity ent, World* wrd)
        : entity{ent}, world{std::make_shared<World>(wrd)} {}
    template <typename Component>
    Component& get() {
        return world->get_comp<Component>(entity);
    }
};

}  // namespace blur