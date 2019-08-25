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

    template <typename... Cs>
    static ComponentMask of() {
        auto cm = ComponentMask();
        (cm.merge<Cs>(), ...);
        return cm;
    }

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

    bool operator==(const ComponentMask& other) const { return other.mask == mask; }

    bool contains(hash_code_t other) { return ((mask & other) == other); }

    bool contains(ComponentMask other) {
        return ((mask & other.mask) == other.mask);
    }
};

struct ComponentMeta {
    using ctor_func_t = void(void*);
    using dtor_func_t = void(void*);

    size_t size{0};
    hash_code_t id{0};
    std::string name{""};
    ctor_func_t* ctor;
    dtor_func_t* dtor;

    ComponentMeta() {}
    ComponentMeta(hash_code_t id, size_t size, std::string name,
                  ctor_func_t* ctor, dtor_func_t* dtor)
        : id{id}, size{size}, name{name}, ctor{ctor}, dtor{dtor} {}

    bool operator==(const ComponentMeta& other) const { return other.id == id; }

    template <typename C>
    static ComponentMeta of() {
        return ComponentMeta(typeid(C).hash_code(), sizeof(C),
                             typeid(C).name() + 1, [](void* p) { new (p) C{}; },
                             [](void* p) { ((C*)p)->~C(); });
    }
};

struct ComponentStorage {
    ComponentMeta component;
    byte* cursor{nullptr};
    ComponentStorage(){};
    ComponentStorage(ComponentMeta component, byte* cursor)
        : cursor{cursor}, component{component} {}

    ComponentStorage& operator=(const ComponentStorage& rhs) {
        component = rhs.component;
        cursor = rhs.cursor;
        return *this;
    }

    void create(unsigned idx) { component.ctor(index_ptr(idx)); }
    void destroy(unsigned idx) { component.dtor(index_ptr(idx)); }
    void* index_ptr(unsigned idx) { return cursor + (idx * component.size); }

    void copy_from(unsigned sidx, unsigned oidx, ComponentStorage& other) {
        memcpy(index_ptr(sidx), other.index_ptr(oidx), component.size);
    }

    template <typename C>
    constexpr C& try_get(size_t idx) {
        auto other = ComponentMeta::of<C>();
        if (component.id != other.id)
            throw std::invalid_argument("Can't get component " + other.name +
                                        " from a " + component.name +
                                        " storage");
        else
            return ((C*)cursor)[idx];
    }
};

}  // namespace blur