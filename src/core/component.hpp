#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>

#include "meta.hpp"

namespace blur {

using byte = char;

struct ComponentStorage {
    MetaComponentBase component;
    byte* cursor{nullptr};
    ComponentStorage(){};
    ComponentStorage(MetaComponentBase component, byte* cursor)
        : cursor{cursor}, component{component} {}
    ComponentStorage& operator=(const ComponentStorage& rhs) {
        component = rhs.component;
        cursor = rhs.cursor;
        return *this;
    }
    void create(unsigned idx) { component.ctor(index_ptr(idx)); }
    void destroy(unsigned idx) { component.dtor(index_ptr(idx)); }
    void* index_ptr(unsigned idx) { return cursor + (idx * component.size); }

    template <typename T>
    constexpr T& try_get(size_t idx) {
        std::string other_name = typeid(T).name() + 1;
        if (component.id.hash != typeid(T).hash_code() &&
            component.size != sizeof(T) && component.name != other_name)
            throw std::invalid_argument("Can't get component " + other_name +
                                        " from a " + component.name +
                                        " storage");
        else
            return ((T*)cursor)[idx];
    }
};

}  // namespace blur