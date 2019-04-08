#pragma once

#define MAX_ENTITIES 3000

#include <cstddef>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

namespace blur {

using EntityId = int32_t;
using block_ptr = std::shared_ptr<ArchetypeBlock>;

struct Entity {
    EntityId id{-1};
    unsigned counter{0};
};

class ArchetypeBlock;

struct EntityData {
    block_ptr block{nullptr};
    unsigned block_index{0};
    unsigned counter{0};
};

struct EntityTable {
    size_t max_entities{0};
    std::vector<EntityData> entities_data;
    std::vector<EntityId> last_deleted;
    EntityId last_free{0};

    EntityTable(unsigned entities = MAX_ENTITIES) {
        max_entities = entities;
        for (int i = 0; i < max_entities; i++) {
            entities_data.push_back(EntityData{});
        }
    }

    template <typename... Cs>
    std::tuple<EntityId, EntityData&> add(block_ptr block) {
        EntityId idx = -1;
        if (last_deleted.size() > 0) {
            idx = last_deleted.back();
            last_deleted.pop_back();
        } else {
            idx = last_free;
            last_free++;
        }
        auto& data = entities_data[idx];
        data.block = block_ptr(block);
        data.block_index = data.block->next_free();
        return {idx, entities_data[idx]};
    }

    void remove(Entity entity) {
        auto data = entities_data[entity.id];
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

    EntityData& lookup(Entity entity) {
        auto& data = entities_data[entity.id];
        if (data.counter != entity.counter) {
            std::ostringstream os_string;
            os_string << "Entity { id: " << entity.id
                      << ", counter: " << entity.counter
                      << " } does not exist/has been destroyed";
            throw std::invalid_argument(os_string.str());
        }
        return data;
    }
};

}  // namespace blur