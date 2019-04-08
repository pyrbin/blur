#pragma once

#include <cstddef>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

#include "component.hpp"
#include "meta.hpp"

namespace blur {

using byte = char;
using block_allocator = std::allocator<byte>;

class ArchetypeBase {
   public:
    virtual ~ArchetypeBase() {}
    hash_code combined_hash;
    std::vector<MetaComponentBase> set;
    ArchetypeBase& operator=(ArchetypeBase& other) {
        combined_hash = other.combined_hash;
        set = std::move(other.set);
    }
};
template <typename... Cs>
struct Archetype : public ArchetypeBase {
   public:
    hash_code combined_hash;
    std::vector<MetaComponentBase> set;

    constexpr Archetype() { (insert_meta_component<Cs>(), ...); }
    constexpr Archetype(const Archetype<Cs...>& rhs) : set{rhs.set} {}
    constexpr Archetype& operator=(const Archetype& rhs) {
        set.clear();
        for (auto c : rhs.set) {
            set.push_back(c);
        }
        return *this;
    }

   private:
    template <typename Component>
    constexpr void insert_meta_component() {
        auto meta = MetaComponent<Component>();
        set.push_back(meta);
        combined_hash += meta.id.hash;
    }
};

struct ArchetypeBlock {
    static const size_t BLOCK_SIZE = 1024 * 16;  // 16kb
    const size_t block_size;
    const size_t max_entities;
    const size_t component_count;
    ComponentStorage* components;
    ArchetypeBase archetype;
    block_allocator alloc;
    byte* data{nullptr};
    unsigned next{0};

    // TODO: archetype doesnt keep set??
    template <typename... Cs>
    explicit ArchetypeBlock(const Archetype<Cs...>& at) noexcept
        : block_size{BLOCK_SIZE},
          archetype{at},
          component_count{at.set.size()},
          max_entities{BLOCK_SIZE /
                       (sizeof(unsigned) + (0 + ... + sizeof(Cs)))} {
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

    unsigned next_free() { return next++; }

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