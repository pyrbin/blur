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
    using systm_t = std::shared_ptr<SystemProcessBase>;
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
        auto sfn = systm_t(new SystemProcess(new Sys()));
        static_cast<SystemProcess<Sys>*>(sfn.get())->set_mem_fn(&Sys::process);
        systems.push_back(sfn);
    }

    void tick() {
        for (auto& sys : systems) {
            for (auto& blk : blocks) {
                auto valid_block =
                    blk->archetype.comp_mask.contains(sys->signature);
                if (valid_block) {
                    for (unsigned i{0}; i < blk->size; i++) {
                        sys->operator()(blk.get(), i);
                    }
                }
            }
        }
    }

    template <typename C>
    constexpr C& get_comp(Entity ent) noexcept {
        auto end = et.lookup(ent);
        auto stg = end.block->get_storage<C>();
        return stg.template try_get<C>(end.block_index);
    }

    template <typename... Cs, typename F>
    void mod_comp(Entity ent, F&& f) {
        return std::forward<F>(f)(get_comp<Cs>(ent)...);
    }

    template <typename Class, typename... Params>
    void mod_comp_helper(Entity ent, Class* obj,
                         void (Class::*f)(Params...) const) {
        //(sys_ptr.get()->*f)(std::forward<Args>(args)...);
        (obj->*f)(get_comp<std::decay_t<Params>>(ent)...);
    }

    // optional overload for mutable lambdas
    template <typename Class, typename... Params>
    void mod_comp_helper(Entity ent, Class* obj, void (Class::*f)(Params...)) {
        (obj->*f)(get_comp<std::decay_t<Params>>(ent)...);
    }

    template <typename Functor>
    void mod_comp_alt(Entity ent, Functor&& f) {
        mod_comp_helper(ent, &f, &std::decay_t<Functor>::operator());
    }

    // optional overload for function pointers
    template <typename... Params>
    void mod_comp_alt(Entity ent, void (*f)(Params...)) {
        f(get_comp<std::decay_t<Params>(ent)>...);
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
    std::vector<cmask_t> block_hashes;
    EntityTable et;

    template <typename... Cs>
    constexpr block_t create_archetype_block(
        const Archetype<Cs...>& arch) noexcept {
        block_t ptr{nullptr};

        const cmask_t mask = arch.comp_mask;

        for (size_t i{0}; i < blocks.size(); i++) {
            if (block_hashes[i] == mask) {
                ptr = blocks[i];
                break;
            }
        }
        if (ptr == nullptr) {
            ptr = std::make_shared<ArchetypeBlock>(BLOCK_SIZE, arch);
            blocks.push_back(ptr);
            block_hashes.push_back(mask);
        }
        return ptr;
    }
};
}  // namespace blur