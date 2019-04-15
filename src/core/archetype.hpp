#pragma once

#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

#include "component.hpp"

namespace blur {

using byte = char;
using block_allocator = std::allocator<byte>;
using component_set = std::vector<MetaComponentBase>;
template <typename H>
using no_ref_const_t =
    typename std::remove_const<std::remove_reference<H>>::type;
template <typename H>
using no_ref_t = typename std::remove_reference<H>::type;

class ArchetypeBase {
   public:
    virtual ~ArchetypeBase() {}
    hash_code_t merged_hash;
    component_set set;
    bool operator==(ArchetypeBase arch) {
        return merged_hash == arch.merged_hash;
    }
};
template <typename... Cs>
struct Archetype : public ArchetypeBase {
   public:
    component_set set;
    hash_code_t merged_hash{0};

    constexpr Archetype() { (insert_meta_component<Cs>(), ...); }
    constexpr Archetype(const Archetype<Cs...>& rhs)
        : merged_hash{rhs.merged_hash}, set{rhs.set} {}
    constexpr Archetype& operator=(const Archetype& rhs) {
        merged_hash = rhs.merged_hash;
        set.clear();
        for (auto c : rhs.set) {
            set.push_back(c);
        }
        return *this;
    }

   private:
    template <typename Component>
    constexpr void insert_meta_component() {
        using comp_t = no_ref_t<Component>;
        auto meta = MetaComponent<comp_t>();
        set.push_back(meta);
        merged_hash += meta.id.hash;
    }
};

struct ArchetypeBlock {
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
          component_count{at.set.size()},
          max_entities{block_size /
                       (sizeof(unsigned) + (0 + ... + sizeof(Cs)))} {
        archetype.merged_hash = at.merged_hash;
        archetype.set = std::move(at.set);
        data = alloc.allocate(block_size);
        byte* cursor = data;
        components = new (cursor) ComponentStorage[component_count];
        cursor += sizeof(ComponentStorage) * component_count;
        unsigned i{0};
        unsigned offset{0};
        for (const auto& meta : at.set) {
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

    unsigned next_free() {
        size = next + 1;
        return next++;
    }

    void initialize_entry(unsigned idx) {}

    template <typename Component>
    ComponentStorage& get_storage() {
        auto meta = MetaComponent<Component>();
        for (unsigned i{0}; i < component_count; i++) {
            if (components[i].component.id.hash == meta.id.hash) {
                return components[i];
            }
        }
    }
};

}  // namespace blur