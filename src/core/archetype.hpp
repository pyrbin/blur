#pragma once

#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

#include "component.hpp"
#include "util.hpp"

namespace blur {

class ArchetypeBase {
   public:
    using comp_mask_t = ComponentMask;
    using comp_meta_t = std::vector<ComponentMetaBase>;
    comp_mask_t comp_mask;
    comp_meta_t comp_meta;
    virtual ~ArchetypeBase() {}
    ArchetypeBase() {}
    ArchetypeBase& operator=(const ArchetypeBase& rhs) {
        comp_mask.mask = rhs.comp_mask.mask;
        comp_meta.clear();
        for (auto& c : rhs.comp_meta) comp_meta.push_back(c);
        return *this;
    }
    bool operator==(ArchetypeBase other) {
        return comp_mask == other.comp_mask;
    }
};
template <typename... Cs>
struct Archetype : public ArchetypeBase {
   public:
    Archetype() { (build_types<Cs>(), ...); }
    Archetype(const Archetype& other) {
        comp_mask = std::move(other.comp_mask);
        comp_meta.clear();
        for (auto& d : other.comp_meta) {
            comp_meta.push_back(std::move(d));
        }
    }

   private:
    template <typename C>
    void build_types() {
        using comp_t = no_ref_t<C>;
        auto meta = ComponentMeta<comp_t>();
        comp_meta.push_back(meta);
        comp_mask.add<comp_t>();
    }
};

struct ArchetypeBlock {
public:
    using byte = char;
    using block_allocator = std::allocator<byte>;

    const size_t block_size;
    const size_t max_entities;
    const size_t component_count;

    ComponentStorage* components;

    ArchetypeBase archetype;
    
    block_allocator alloc;
    
    byte* data{nullptr};
    
    unsigned next{0};

    // Make iterator class
    // do get all occupied entities in an array
    unsigned size{0};

    // TODO: archetype doesnt keep set??
    template <typename... Cs>
    explicit ArchetypeBlock(size_t block_size, const Archetype<Cs...>& at)
        : block_size{block_size},
          component_count{at.comp_meta.size()},
          max_entities{block_size /
                       (sizeof(unsigned) + (0 + ... + sizeof(Cs)))} {
        archetype = Archetype<Cs...>(at);
        data = alloc.allocate(block_size);
        byte* cursor = data;
        components = new (cursor) ComponentStorage[component_count];
        cursor += sizeof(ComponentStorage) * component_count;
        unsigned i{0};
        unsigned offset{0};
        for (const auto& meta : archetype.comp_meta) {
            components[i] = ComponentStorage(meta, cursor + offset);
            offset += meta.size * max_entities;
            i++;
        }
    }

    ~ArchetypeBlock() {
        alloc.deallocate(data, block_size);
        data = nullptr;
    }

    // ArchetypeBlock(ArchetypeBlock&) = delete;
    // ArchetypeBlock& operator=(ArchetypeBlock&) = delete;
    ArchetypeBlock(ArchetypeBlock&& other)
        : max_entities{other.max_entities},
          block_size{other.block_size},
          component_count{other.component_count} {
        archetype = other.archetype;
        data = other.data;
        other.data = nullptr;
    }
    // ArchetypeBlock& operator=(ArchetypeBlock&&) = delete;

    unsigned insert_new() {
        size = next + 1;
        return next++;
    }

    void initialize_entry(unsigned idx) {}

    
    // Functor objects
    template <typename Functor>
    void mod_entries(Functor&& f) {
        mod_entries_inner(&f, &std::decay_t<Functor>::operator());
    }

    template <typename Component>
    Component& get_entry(unsigned idx) {
        auto& storage = get_storage<Component>();
        return storage.template try_get<Component>(idx);
    }

    template <typename Component>
    ComponentStorage& get_storage() {
        auto meta = ComponentMeta<Component>();
        for (unsigned i{0}; i < component_count; i++) {
            if (components[i].component.id == meta.id) {
                return components[i];
            }
        }
    }
    
private:
    template <typename Class, typename... Args>
    void mod_entries_inner(Class* obj, void (Class::*f)(Args...) const) {
        for(unsigned i{0}; i < size; i++) {
            (obj->*f)(get_entry<std::decay_t<Args>>(i)...);            
        }
    }

};

}  // namespace blur