#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>

namespace blur {

using byte = char;
using hash_code_t = size_t;

struct MetaComponentId {
    hash_code_t hash{0};
};

struct MetaComponentBase {
    using ctor_func_t = void(void*);
    using dtor_func_t = void(void*);

    size_t size{0};
    MetaComponentId id{};
    std::string name{""};
    ctor_func_t* ctor;
    dtor_func_t* dtor;

    MetaComponentBase() {}
    MetaComponentBase(MetaComponentId id, size_t size, std::string name,
                      ctor_func_t* ctor, dtor_func_t* dtor)
        : id{id}, size{size}, name{name}, ctor{ctor}, dtor{dtor} {}
};
template <typename C>
struct MetaComponent : public MetaComponentBase {
    constexpr MetaComponent()
        : MetaComponentBase({typeid(C).hash_code()}, sizeof(C),
                            typeid(C).name() + 1, [](void* p) { new (p) C{}; },
                            [](void* p) { ((C*)p)->~C(); }) {}
};

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