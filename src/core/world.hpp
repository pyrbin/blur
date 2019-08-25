#pragma once

#define MAX_ENTITIES 1000000
#define BLOCK_SIZE 1024 * 64

#include <memory>
#include <tuple>
#include <algorithm>
//#include <execution>

#include "archetype.hpp"
#include "entity_table.hpp"
#include "system.hpp"
#include "util.hpp"

namespace blur {

class World {
   public:
    using block_storage_t = std::shared_ptr<ArchetypeBlockStorage>;
    using systm_t = std::shared_ptr<SystemFunctorBase>;
    using block_t = std::shared_ptr<ArchetypeBlock>;
    using cmask_t = ComponentMask;

    World() : et{EntityTable(MAX_ENTITIES)} {}

    template <typename... Cs>
    Entity create() noexcept {
        return create(Archetype::of<Cs...>());
    }

    Entity create(const Archetype& arch) noexcept {
        auto block = create_archetype_storage(arch);
        auto [idx, ed] = et.add(block->find_free());
        return { idx, ed.counter };
    }
    
    void remove(Entity ent) noexcept {
        auto& entry = et.lookup(ent);
        auto block_index = entry.block_index;
        et.remove(ent);
    }

    void remove(std::vector<Entity> ents...) noexcept {
        for(auto& e : ents) remove(e);
    }

    template <typename... Cs>
    std::vector<Entity> batch(unsigned size) noexcept {
        return batch(size, Archetype::of<Cs...>());
    }

    std::vector<Entity> batch(unsigned size, const Archetype& arch) noexcept {
        std::vector<Entity> spawned;
        for (auto i{0}; i < size; i++)
            spawned.push_back(create(arch));
        return spawned;
    }

    template <typename Sys, typename... Args>
    void insert(Args&&... args) {
        static_assert(has_process_fn<Sys>(), "Systems requires a valid process function");
        auto uptr = systm_t(new SystemFunctor(new Sys(std::forward<Args>(args)...)));
        static_cast<SystemFunctor<Sys>*>(uptr.get())->init(&Sys::process);
        systems.push_back(uptr);
    }

    void tick() {
        for (auto& block : blocks) {
            for (auto& sys : systems) {
                if (sys.get()->valid_mask(block->archetype.comp_mask)) {
                    // TODO: fix iteration
                    for(auto&& b : block->blocks) {
                        auto tot_size{b->size()};
                        auto tot_found{0};
                        for(auto i{0}; i < b->max_entries; i++) {
                            if(b->entities[i].id == -1) continue;
                            sys->operator()(i, b.get());
                            tot_found++;
                        }
                    }
                }
            }
        }
    }

    // Get component by entity entry
    template <typename C>
    constexpr C& get_comp(Entity ent) noexcept {
        return get_modifiable_comp<C>(ent);
    }

    // Get component by entity entry
    template <typename C>
    constexpr const C& read_comp(Entity ent) noexcept {
        return std::as_const(get_modifiable_comp<C>(ent));
    }

    // Functor objects
    template <typename Functor>
    void mod_comp(Entity ent, Functor&& f) {
        mod_comp_helper(ent, &f, &std::decay_t<Functor>::operator());
    }

    template <typename... Cs>
    void add_comp(Entity ent) {
        auto arch = et.lookup(ent).block->archetype;
        (arch.add<Cs>(), ...);
        update_archetype(ent, arch);
    }

    template <typename... Cs>
    void del_comp(Entity ent) {
        auto arch = et.lookup(ent).block->archetype;
        (arch.remove<Cs>(), ...);
        update_archetype(ent, arch);
    }

    template <typename... Cs>
    bool has_comp(Entity ent) {
        auto& data = et.lookup(ent);
        auto& arch = data.block->archetype;
        return arch.comp_mask.contains(ComponentMask::of<Cs...>());
    }

    void print(std::ostream& os) {
        for(auto&& bs : blocks) {
            bs->print(os);
        }
    }

   private:
    std::vector<block_storage_t> blocks;
    std::vector<systm_t> systems;
    EntityTable et;

    void update_archetype(Entity ent, const Archetype& new_arch) noexcept {
        auto& old_data = et.lookup(ent);
        auto old_block = old_data.block;
        auto old_idx = old_data.block_index;

        auto block_stg { create_archetype_storage(new_arch) };
        auto block = block_stg->find_free();
        
        if (block->archetype == old_block->archetype) return;
        auto [idx, data] = et.insert_to_block(ent.id, block);
        block->transfer_from(old_block.get(), old_idx, idx);
        old_block->remove(old_idx);
    }

    block_storage_t create_archetype_storage(const Archetype& arch) noexcept {
        block_storage_t ptr{find_block_storage(arch)};
        if (ptr == nullptr) {
            ptr = std::make_shared<ArchetypeBlockStorage>(BLOCK_SIZE, arch);
            blocks.push_back(ptr);
        }
        return ptr;
    }

    block_storage_t find_block_storage(const Archetype& arch) {
        const cmask_t mask = arch.comp_mask;
        for (auto& block : blocks) {
            if (block->archetype.comp_mask == mask) {
                return block;
            }
        }
        return nullptr;
    }
    

    // https://stackoverflow.com/questions/55756181/use-lambda-to-modify-references-identified-by-a-packed-parameter
    template <typename Class, typename... Args>
    void mod_comp_helper(Entity ent, Class* obj,
                         void (Class::*f)(Args...) const) {
        (obj->*f)(get_modifiable_comp<std::decay_t<Args>>(ent)...);
    }

    // optional overload for mutable lambdas
    template <typename Class, typename... Args>
    void mod_comp_helper(Entity ent, Class* obj, void (Class::*f)(Args...)) {
        (obj->*f)(get_modifiable_comp<std::decay_t<Args>>(ent)...);
    }

    template <typename C>
    constexpr C& get_modifiable_comp(Entity ent) noexcept {
        auto entry = et.lookup(ent);
        return entry.block->get_entry<C>(entry.block_index);
    }
};

}  // namespace blur