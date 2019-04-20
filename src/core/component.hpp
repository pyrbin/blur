#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>

#include "util.hpp"

namespace blur {

using byte = char;
using hash_code_t = unsigned;

struct ComponentMask {
    hash_code_t mask{0};
    template <typename C>
    hash_code_t add() {
        using comp_t = no_ref_t<C>;
        mask |= (1 << typeid(comp_t).hash_code());
        return mask;
    }
    template <typename... Cs>
    hash_code_t merge() {
        (add<Cs>(), ...);
        return mask;
    }
    template <typename C>
    hash_code_t remove() {
        using comp_t = no_ref_t<C>;
        mask &= ~(1 << typeid(comp_t).hash_code());
        return mask;
    }
    template <typename... Cs>
    hash_code_t slice() {
        (remove<Cs>(), ...);
        return mask;
    }
    bool operator==(ComponentMask other) { return other.mask == mask; }
    bool contains(ComponentMask other) {
        return ((mask & other.mask) == other.mask);
    }
};
template <typename... Cs>
struct ImmutableComponentMask : public ComponentMask {
    const hash_code_t mask{0};
    constexpr ImmutableComponentMask() : mask{merge<Cs...>()} {}
};

struct ComponentMetaBase {
    using ctor_func_t = void(void*);
    using dtor_func_t = void(void*);

    size_t size{0};
    hash_code_t id{0};
    std::string name{""};
    ctor_func_t* ctor;
    dtor_func_t* dtor;

    ComponentMetaBase() {}
    virtual ~ComponentMetaBase() {}
    ComponentMetaBase(hash_code_t id, size_t size, std::string name,
                      ctor_func_t* ctor, dtor_func_t* dtor)
        : id{id}, size{size}, name{name}, ctor{ctor}, dtor{dtor} {}
};
template <typename C>
struct ComponentMeta : public ComponentMetaBase {
    constexpr ComponentMeta()
        : ComponentMetaBase(typeid(C).hash_code(), sizeof(C),
                            typeid(C).name() + 1, [](void* p) { new (p) C{}; },
                            [](void* p) { ((C*)p)->~C(); }) {}
};

struct ComponentStorage {
    ComponentMetaBase component;
    byte* cursor{nullptr};
    ComponentStorage(){};
    ComponentStorage(ComponentMetaBase component, byte* cursor)
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
        if (component.id != typeid(T).hash_code() &&
            component.size != sizeof(T) && component.name != other_name)
            throw std::invalid_argument("Can't get component " + other_name +
                                        " from a " + component.name +
                                        " storage");
        else
            return ((T*)cursor)[idx];
    }
};

}  // namespace blur