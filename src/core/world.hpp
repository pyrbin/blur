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

    World() : et{EntityTable(10000)} {}
    template <typename... Cs>
    constexpr Entity create() noexcept {
        return create(Archetype<Cs...>());
    }

    template <typename... Cs>
    constexpr Entity create(const Archetype<Cs...>& arch) noexcept {
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
            for (auto& block : blocks){
                if(sys.get()->valid_mask(block->archetype.comp_mask)) {
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
        // add component
    }

    template <typename... Cs>
    void del_comp(Entity ent) {
        static_assert(has_comp<Cs...>(ent),
                      "TODO: add good error msg (no has comp to delete)");
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

    template <typename... Cs>
    constexpr block_t create_archetype_block(
        const Archetype<Cs...>& arch) noexcept {
        block_t ptr{nullptr};

        const cmask_t mask = arch.comp_mask;

        for (auto& block : blocks) {
            if (block->archetype.comp_mask == mask) {
                ptr = block;
                break;
            }
        }
        if (ptr == nullptr) {
            ptr = std::make_shared<ArchetypeBlock>(BLOCK_SIZE, arch);
            blocks.push_back(ptr);
        }
        return ptr;
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