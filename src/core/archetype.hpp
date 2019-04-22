#pragma once

#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>
#include <cstring>
#include <unordered_map>
#include <vector>

#include "component.hpp"
#include "entity.hpp"
#include "util.hpp"

namespace blur {

class Entity;

class Archetype {
   public:
    using comp_mask_t = ComponentMask;
    using comp_meta_t = std::vector<ComponentMeta>;
    comp_mask_t comp_mask;
    comp_meta_t comp_meta;

    Archetype(){}

    template <typename... Cs>
    static Archetype of() {
        auto arch = Archetype();
        (arch.add_no_sort<Cs>(), ...);
        arch.sort();
        return arch;
    }

    Archetype(const Archetype& other) {
        comp_mask = std::move(other.comp_mask);
        comp_meta.clear();
        for (auto& d : other.comp_meta) {
            comp_meta.push_back(std::move(d));
        }
        sort();
    }

    Archetype& operator=(const Archetype& rhs) {
        comp_mask.mask = rhs.comp_mask.mask;
        comp_meta.clear();
        for (auto c : rhs.comp_meta) comp_meta.push_back(c);
        sort();
        return *this;
    }

    bool operator==(Archetype other) {
        return comp_mask == other.comp_mask;
    }

    template <typename C>
    void add() {
        add_no_sort<C>();
        sort();
    }
private:
    void sort() {
        using cm_t = ComponentMeta;
        std::sort(comp_meta.begin(), comp_meta.end(), [](cm_t& a, cm_t& b) {
            return a.id > b.id;   
        });
    }
    template <typename C>
    void add_no_sort() {
        using comp_t = no_ref_t<C>;
        if(comp_mask.contains(comp_mask_t::of<C>())) return;
        auto meta = ComponentMeta::of<comp_t>();
        comp_meta.push_back(meta);
        comp_mask.add<comp_t>();
    }
};

struct ArchetypeBlock {
public:
    using byte = char;
    using block_allocator = std::allocator<byte>;

    size_t block_size;
    size_t max_entities;
    size_t component_count;
    Archetype archetype;

    std::vector<Entity> entities;
    ComponentStorage* components;
    
    block_allocator alloc;
    
    byte* data{nullptr};
    
    unsigned next{0};


    // TODO: archetype doesnt keep set??
    template <typename... Cs>
    explicit ArchetypeBlock(size_t block_size, const Archetype& at)
        : block_size{block_size},
          archetype{Archetype(at)},
          component_count{at.comp_meta.size()} {

        max_entities = block_size /
                       (sizeof(Entity) + (0 + ... + sizeof(Cs)));
                       
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

    unsigned insert(Entity ent) {
        entities.push_back(ent);
        for (unsigned i{0}; i < component_count; i++) {
            components[i].create(next);
        }
        return next++;
    }


    void cpy_from(unsigned idx, ArchetypeBlock* ab, unsigned other){
        for (unsigned i{0}; i < ab->component_count; i++) {
            auto& other_c = ab->components[i];
			for (unsigned j{0}; j < component_count; j++) 
			{
				auto& this_c = components[j];
				if (this_c.component.id == other_c.component.id)
				{

				}
            }
        }
    }
    


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
        auto meta = ComponentMeta::of<Component>();
        for (unsigned i{0}; i < component_count; i++) {
            if (components[i].component.id == meta.id) {
                return components[i];
            }
        }
    }
    
private:
    template <typename Class, typename... Args>
    void mod_entries_inner(Class* obj, void (Class::*f)(Args...) const) {
        for(unsigned i{0}; i < entities.size(); i++) {
            (obj->*f)(get_entry<std::decay_t<Args>>(i)...);         
        }
    }

};

}  // namespace blur