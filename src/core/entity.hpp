#pragma once

#include <cstddef>
#include <iomanip>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

namespace blur {

using EntityId = int32_t;

struct Entity {
    EntityId id{-1};
    unsigned counter{0};
};

}