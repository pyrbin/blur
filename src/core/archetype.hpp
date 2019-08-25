#pragma once

#include <cstddef>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>
#include <iterator>
#include <stdexcept>
#include <numeric>
#include <cassert>
#include <set>

#include "component.hpp"
#include "entity.hpp"
#include "util.hpp"

#include "pre_allocator.hpp"

namespace blur {

class Entity;

class Archetype {
   public:
    using comp_mask_t = ComponentMask;
    using comp_meta_t = std::vector<ComponentMeta>;
    
    unsigned size{0};
    comp_mask_t comp_mask;
    comp_meta_t comp_meta;

    Archetype() {}

    template <typename... Cs>
    static Archetype of() {
        auto arch = Archetype();
        (arch.add<Cs>(), ...);
        return arch;
    }

    Archetype(const Archetype& other) {
        comp_mask = std::move(other.comp_mask);
        comp_meta.clear();
        for (auto& d : other.comp_meta)
            comp_meta.push_back(std::move(d));
        update_order();
    }

    Archetype& operator=(const Archetype& rhs) {
        comp_mask.mask = rhs.comp_mask.mask;
        comp_meta.clear();
        for (auto c : rhs.comp_meta)
            comp_meta.push_back(c);
        update_order();
        return *this;
    }

    bool operator==(const Archetype& other) const { return comp_mask == other.comp_mask; }

    template <typename C>
    void add() {
        using comp_t = no_ref_t<C>;
        if (comp_mask.contains(comp_mask_t::of<comp_t>())) return;
        auto meta = ComponentMeta::of<comp_t>();
        comp_meta.push_back(meta);
        comp_mask.add<comp_t>();
        update_order();
    }

    template <typename C>
    void remove() {
        using comp_t = no_ref_t<C>;
        if (!comp_mask.contains(comp_mask_t::of<comp_t>())) return;
        auto meta = ComponentMeta::of<comp_t>();
        comp_meta.erase(std::find(comp_meta.cbegin(), comp_meta.cend(), meta));
        comp_mask.remove<comp_t>();
        update_order();
    }

   private:
    void update_order() {
        using cm_t = ComponentMeta;
        std::sort(comp_meta.begin(), comp_meta.end(),
                  [](cm_t& a, cm_t& b) { return a.id > b.id; });
        auto meta_size = [](int sum, ComponentMeta b) { return sum + b.size; };
        size = std::accumulate(comp_meta.begin(), comp_meta.end(), 0, meta_size);
    }
};

struct ArchetypeBlock {
   public:
    using byte = char;
    using block_allocator = std::allocator<byte>;
    using occupied_storage = std::set<size_t, std::less<size_t>, pre_allocator<size_t>>;
    using entity_storage = Entity*;
    using component_storage = ComponentStorage*;

    const size_t block_size;
    const size_t component_count;
    const Archetype archetype;
    
    size_t max_entries;

    occupied_storage* occupied;

    explicit ArchetypeBlock(size_t block_size, const Archetype& at)
        : block_size{block_size},
          archetype{Archetype(at)},
          component_count{at.comp_meta.size()} {

        data = alloc.allocate(block_size);
        byte* cursor = data;

        unsigned entry_size = sizeof(Entity) + archetype.size;
		unsigned comp_header_size = sizeof(ComponentStorage) * component_count;
        
        max_entries = ((block_size - comp_header_size) / entry_size) - 2;
        
		entities = new (cursor) Entity[max_entries];
		cursor += sizeof(Entity) * max_entries;

        occupied = new occupied_storage(max_entries);
        cursor += sizeof(size_t) * max_entries;

        components = new (cursor) ComponentStorage[component_count];
        cursor += comp_header_size;

        unsigned i{0}, offset{0};
        for (const auto& m : archetype.comp_meta) {
            components[i] = ComponentStorage(m, cursor + offset);
            offset += m.size * (max_entries + 1);
            i++;
        }
    }

    ~ArchetypeBlock() {
        alloc.deallocate(data, block_size);
        data = nullptr;
    }

    bool full() {
        return space() <= 0;
    }

    unsigned space() {
        return max_entries - size();
    }

    unsigned size() {
        return last;
    }

    unsigned insert(Entity ent) {
        assert(last <= max_entries);

        entities[last] = ent;

        for (unsigned i{0}; i < component_count; i++) {
            components[i].create(last);
        }

        last++;
        
        return last++;
    }

    unsigned remove(unsigned idx) {
        assert(idx < max_entries);
        for (unsigned i{0}; i < component_count; i++)
            components[i].destroy(idx);
        entities[idx].id = -1;
        occupied[]
        return last;
    }

    void transfer_from(ArchetypeBlock* block, unsigned from, unsigned src) {
        assert(src < max_entries);
        for (unsigned i{0}; i < component_count; i++) {
            for (unsigned j{0}; j < block->component_count; j++) {
                auto is_valid = block->components[j].component.id ==
                                components[i].component.id;
                if (is_valid) {
                    components[i].copy_from(src, from, components[j]);
                }
            }
        }
    }

    void for_each_storage(std::function<void(ComponentStorage&)> f) {
        for (unsigned i{0}; i < component_count; i++) f(components[i]);
    }

    template <typename Functor>
    void modify_components(unsigned idx, Functor&& f) {
        assert(idx < max_entries);
        modify_components_inner(idx, &f, &std::decay_t<Functor>::operator());
    }

    template <typename Component>
    Component& get_entry(unsigned idx) {
        assert(idx < max_entries);
        assert(entities[idx].id >= 0);
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
        // TODO: probly not throw here, bad idea
        std::ostringstream os_string;
        os_string << "Entity does not have that component@" << meta.name;
        throw std::invalid_argument(os_string.str());
    }

   private:
    unsigned last{0};

    block_allocator alloc;
    component_storage components;
    entity_storage entities;
    byte* data{nullptr};

    template <typename Class, typename... Args>
    void modify_components_inner(unsigned idx, Class* obj, void (Class::*f)(Args...) const) {
        (obj->*f)(get_entry<std::decay_t<Args>>(idx)...);
    }
};

class ArchetypeBlockStorage {
public:
    using block_t = std::shared_ptr<ArchetypeBlock>;

	Archetype archetype;
	size_t block_size;
    
    unsigned total_blocks{0};

    std::vector<block_t> blocks;
    block_t cached_free{nullptr};

    explicit ArchetypeBlockStorage(size_t block_size, const Archetype& at)
        : block_size{block_size},
          archetype{Archetype{at}}{}
	
    block_t find_free(){
        if (cached_free && !cached_free->full())
            return cached_free;
        
        auto it = std::find_if(blocks.begin(), blocks.end(), [](auto& b){
            return !b->full();
        });

        block_t ptr{nullptr};

        if (it != blocks.end()) {
            ptr = *it;
        } else {
            ptr = create_block();
        }

        cached_free = ptr;
        return ptr;
    }

    void remove(block_t* ab) {

    }

    void print(std::ostream& os){
        auto i{0};
        std::ostringstream oss;
        oss << "==========================================\n";
        oss << "| Block Storage | " << archetype.comp_mask.mask << "\n";
        oss << "==========================================\n";
        for(auto&& b : blocks) {
            oss << "[" << i++ << "]\n";
            oss << "  Max: " << b->max_entries << "\n";
            oss << "  Curr: " << b->size() << "\n";
            oss << "  Full: " << ((1-(float)b->space()/b->max_entries))*100 << "%\n";
            oss << "  Comps: " << b->component_count << "\n";
            oss << "------------------------------------------\n";
        }
        os << oss.str();
    }

private:
    //privates
    block_t create_block() {
        auto ptr = std::make_shared<ArchetypeBlock>(block_size, archetype);
        blocks.push_back(ptr);
        return ptr;
    }
};

}  // namespace blur