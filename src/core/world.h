#pragma once

#define MAX_ENTITIES 10000
#define BLOCK_SIZE 1024 * 64

#include <memory>

#include "archetype.hpp"
#include "entity.hpp"
#include "meta.hpp"
#include "system.hpp"

namespace blur {

using systm_t = SystemProcessBase;
using block_t = std::shared_ptr<ArchetypeBlock>;

class World {
   public:
    World();
    template <typename... Cs>
    Entity create();
    template <typename... Cs>
    Entity create(const Archetype<Cs...>& arch);
    template <typename Sys, typename... Args>
    void insert(Args... args);
    void update();
    template <typename Component>
    Component& get_comp(Entity ent);

   private:
    std::vector<systm_t> systems;
    std::vector<block_t> blocks;
    std::vector<hash_code_t> block_hashes;
    EntityTable et;
    template <typename... Cs>
    block_t create_archetype_block(const Archetype<Cs...>& arch);
};

}  // namespace blur