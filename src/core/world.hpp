#pragma once

#define MAX_ENTITIES 10000
#define BLOCK_SIZE 1024 * 64

#include <memory>
#include <tuple>

#include "archetype.hpp"
#include "entity.hpp"
#include "system.hpp"
#include "util.hpp"

namespace blur {

class World {
   public:
    using systm_t = std::shared_ptr<SystemFunctorBase>;
    using block_t = std::shared_ptr<ArchetypeBlock>;
    using cmask_t = ComponentMask;

    World() : et{EntityTable(MAX_ENTITIES)} {}

    template <typename... Cs>
    Entity create() noexcept {
        return create(Archetype::of<Cs...>());
    }

    Entity create(const Archetype& arch) noexcept {
        auto block = create_archetype_block(arch);
        auto [idx, ed] = et.add(block);
        return {idx, ed.counter};
    }

    template <typename Sys, typename... Args>
    void insert(Args... args) {
        static_assert(has_process_fn<Sys>(),
                      "Systems requires a valid process function");
        auto uptr = systm_t(new SystemFunctor(new Sys()));
        static_cast<SystemFunctor<Sys>*>(uptr.get())->init(&Sys::process);
        systems.push_back(uptr);
    }

    void tick() {
        for (auto& sys : systems) {
            for (auto& block : blocks) {
                std::cout << block->archetype.comp_mask.mask << "\n";
                if(sys.get()->valid_mask(block->archetype.comp_mask)) {
                    std::cout << block.get()->component_count << "\n";
                    sys->operator()(block.get());
                }
            }
        }
    }


    // Get component by entity entry
    template <typename C>
    constexpr C& get_comp(Entity ent) noexcept {
        auto da = et.lookup(ent);
        return da.block->get_entry<C>(da.block_index);
    }

    // Functor objects
    template <typename Functor>
    void mod_comp(Entity ent, Functor&& f) {
        mod_comp_helper(ent, &f, &std::decay_t<Functor>::operator());
    }

    template <typename... Cs>
    void add_comp(Entity ent) {
        //assert(has_comp<Cs...>(ent),
        //              "TODO: add good error msg (no has comp to delete)");

        auto& old_data = et.lookup(ent);
        auto old_block = old_data.block;
        auto old_idx = old_data.block_index;
        auto arch = old_block->archetype;

        (arch.add<Cs>(), ...);

        auto block = create_archetype_block(arch);
        std::cout << "sosksk\n";
        
        if(block->archetype == old_block->archetype) return;
            std::cout << "sosksssk\n";

        auto [idx, data] = et.insert_to_block(ent.id, block);
                    std::cout << "sosksssk\n";

        block->cpy_from(data.block_index, old_block.get(), old_idx);
        old_block->shrink();
    }

    template <typename... Cs>
    void del_comp(Entity ent) {
        // static_assert(!has_comp<Cs...>(ent),
        //              "TODO: add good error msg (no has comp to delete)");
        auto& data = et.lookup(ent);
        auto& arch = data.block->archetype;
    }

    template <typename... Cs>
    bool has_comp(Entity ent) {
        auto& data = et.lookup(ent);
        auto& arch = data.block->archetype;
        return arch.comp_mask.contains(ImmutableComponentMask<Cs...>());
    }

   private:
    std::vector<systm_t> systems;
    std::vector<block_t> blocks;
    EntityTable et;

    block_t reallocate_entity(Entity ent, const Archetype& arch) noexcept {
        block_t ptr{find_block(arch)};
        if (ptr == nullptr) return create_archetype_block(arch);
        return ptr;
    }

    block_t create_archetype_block(const Archetype& arch) noexcept {
        
        block_t ptr{find_block(arch)};
        if (ptr == nullptr) {
            ptr = std::make_shared<ArchetypeBlock>(BLOCK_SIZE, arch);
            blocks.push_back(ptr);
        }
        return ptr;
    }

    block_t find_block(const Archetype& arch) {
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
        (obj->*f)(get_comp<std::decay_t<Args>>(ent)...);
    }
    
    // optional overload for mutable lambdas
    template <typename Class, typename... Args>
    void mod_comp_helper(Entity ent, Class* obj, void (Class::*f)(Args...)) {
        (obj->*f)(get_comp<std::decay_t<Args>>(ent)...);
    }
};

}  // namespace blur