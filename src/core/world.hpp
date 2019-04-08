#pragma once

#include <memory>

#include "archetype.hpp"
#include "entity.hpp"

namespace blur {

using block_ptr = std::shared_ptr<ArchetypeBlock>;
using system = size_t;

struct World {
    std::vector<system> systems;
    std::vector<block_ptr> blocks;
    std::vector<hash_code> block_hashes;
    EntityTable et;

    World() : et{EntityTable()} {}

    template <typename... Cs>
    Entity create() {
        return create(Archetype<Cs...>());
    }

    template <typename... Cs>
    Entity create(const Archetype<Cs...>& arch) {
        auto block = create_archetype_block(arch);
        auto [idx, ed] = et.add(block);
        return {idx, ed.counter};
    }

    template <typename Component>
    Component& get_comp(Entity ent) {
        auto end = et.lookup(ent);
        auto stg = end.block->get_storage<Component>();
        return stg.template try_get<Component>(end.block_index);
    }

    template <typename... Cs>
    block_ptr create_archetype_block(const Archetype<Cs...>& arch) {
        block_ptr ptr{nullptr};
        const hash_code hash = arch.combined_hash;
        for (size_t i{0}; i < blocks.size(); i++) {
            if (block_hashes[i] == hash) {
                ptr = blocks[i];
                break;
            }
        }
        if (ptr == nullptr) {
            ptr = std::make_shared<ArchetypeBlock>(std::move(arch));
            blocks.push_back(ptr);
            block_hashes.push_back(hash);
        }
        return ptr;
    }
};

}  // namespace blur