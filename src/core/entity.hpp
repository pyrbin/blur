#pragma once

#include <cstddef>
#include <iomanip>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

namespace blur {

class ArchetypeBlock;

using EntityId = int32_t;
using block_t = std::shared_ptr<ArchetypeBlock>;

struct Entity {
    EntityId id{-1};
    unsigned counter{0};
};

struct EntityData {
    block_t block{nullptr};
    unsigned block_index{0};
    unsigned counter{0};
};

struct EntityTable {
public:
    using instert_ret_t = std::tuple<EntityId, EntityData&>;
    size_t max_entities{0};
    std::vector<EntityData> entities_data;
    std::vector<EntityId> last_deleted;
    EntityId last_free{0};

    EntityTable(unsigned entities = 3000) {
        max_entities = entities;
        for (int i = 0; i < max_entities; i++) {
            entities_data.push_back(EntityData{});
        }
    }

    template <typename... Cs>
    instert_ret_t add(block_t block) {
        return insert_to_block(get_next_free_id(), block);
    }

    template <typename... Cs>
    instert_ret_t insert_to_block(EntityId idx, block_t block) {
        auto& data = entities_data[idx];
        data.block = block_t(block);
                    std::cout << (data.block.get() == nullptr) << "\n";

        data.block_index = data.block->insert_new();
                    std::cout << "soskssskend\n";

        return {idx, data};
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
private:
    EntityId get_next_free_id(){
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