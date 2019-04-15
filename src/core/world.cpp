#include "world.h"

namespace blur {

World::World() : et{EntityTable(10000)} {}

template <typename... Cs>
Entity World::create() {
    return create(Archetype<Cs...>());
}

template <typename... Cs>
Entity World::create(const Archetype<Cs...>& arch) {
    auto block = create_archetype_block(arch);
    auto [idx, ed] = et.add(block);
    return {idx, ed.counter};
}

template <typename Sys, typename... Args>
void World::insert(Args... args) {
    static_assert(has_process_fn<Sys>(),
                  "Systems requires a valid process function");
    auto sfn = SystemProcess(new Sys(), &Sys::process);
    systems.push_back(sfn);
}

void World::update() {
    /*
    for (unsigned s{0}; s < systems.size(); s++) {
        std::vector<block_t> bs;
        for (auto& b : blocks) {
            auto ok = true;  // b->archetype.merged_hash == sys.hash();
            if (ok) {
                for (unsigned i{0}; i < b->size; i++) {
                    (&systems.at(i))->operator()(b.get(), i);
                }
            }
        }
    }*/
}

template <typename Component>
Component& World::get_comp(Entity ent) {
    auto end = et.lookup(ent);
    auto stg = end.block->get_storage<Component>();
    return stg.template try_get<Component>(end.block_index);
}

template <typename... Cs>
block_t World::create_archetype_block(const Archetype<Cs...>& arch) {
    block_t ptr{nullptr};
    const hash_code_t hash = arch.merged_hash;
    for (size_t i{0}; i < blocks.size(); i++) {
        if (block_hashes[i] == hash) {
            ptr = blocks[i];
            break;
        }
    }
    if (ptr == nullptr) {
        ptr = std::make_shared<ArchetypeBlock>(BLOCK_SIZE, std::move(arch));
        blocks.push_back(ptr);
        block_hashes.push_back(hash);
    }
    return ptr;
}

}  // namespace blur