#pragma once

#include <cstddef>
#include <iomanip>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

#include "entity.hpp"

namespace blur {

class ArchetypeBlock;

using block_t = std::shared_ptr<ArchetypeBlock>;

struct EntityEntry {
    block_t block{nullptr};
    unsigned block_index{0};
    unsigned counter{0};
};

struct EntityTable {
   public:
    using insert_return_t = std::tuple<EntityId, EntityEntry&>;
    size_t max_entities{0};
    std::vector<EntityEntry> entities;
    std::vector<EntityId> last_deleted;
    EntityId last_free{0};

    EntityTable(unsigned max_entities) 
        : max_entities{max_entities} {
        for (int i = 0; i < max_entities; i++) {
            entities.push_back(EntityEntry{});
        }
    }

    template <typename... Cs>
    insert_return_t add(block_t block) {
        return insert_to_block(get_next_free_id(), block);
    }

    template <typename... Cs>
    insert_return_t insert_to_block(EntityId idx, block_t block) {
        auto& data = entities[idx];
        data.block = block_t(block);
        data.block_index = data.block->insert({idx, data.counter});
        return {idx, data};
    }

    void remove(Entity entity) {
        auto data = entities[entity.id];
        if (entity.counter == data.counter) {
            // the entity still alive
            data.counter++;
            data.block = nullptr;
            last_deleted.push_back(entity.id);
        } else {
            // the entity is from an older version
            // already destroyed
        }
    }

    EntityEntry& lookup(Entity entity) {
        auto& data = entities[entity.id];
        if (data.counter != entity.counter) {
            std::ostringstream os_string;
            os_string << "Entity { id: " << entity.id
                      << ", counter: " << entity.counter
                      << " } does not exist/has been destroyed";
            throw std::invalid_argument(os_string.str());
        }
        return data;
    }

   private:
    EntityId get_next_free_id() {
        EntityId idx = -1;
        if (last_deleted.size() > 0) {
            idx = last_deleted.back();
            last_deleted.pop_back();
        } else {
            idx = last_free;
            last_free++;
        }
        return idx;
    }
};

}  // namespace blur