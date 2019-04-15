#pragma once

#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>

#include "archetype.hpp"
#include "entity.hpp"

namespace blur {

class Entity;
class ArchetypeBlock;
template <typename... Cs>
class Archetype;

using hash_code_t = size_t;
using hash_list = std::vector<MetaComponentBase>&;

template <typename... Args>
using mem_fn_t = std::function<void(Args...)>;

template <typename System>
using system_ptr_t = std::unique_ptr<System>;

using proc_fn_t = std::function<void(ArchetypeBlock*, unsigned)>;
template <typename H>
using no_ref_t = typename std::remove_reference<H>::type;

struct SystemProcessBase {
   public:
    // ArchetypeBase archetype;
    virtual ~SystemProcessBase() {}
    virtual hash_code_t hash(){};
    virtual void operator()(ArchetypeBlock* bl, unsigned idx){};
};

template <typename S, typename R = void, typename... Args>
struct SystemProcess : public SystemProcessBase {
   public:
    hash_code_t merged_hash;
    proc_fn_t proc_fn;
    system_ptr_t<S> sys_ptr;

    SystemProcess(S* sys, R (S::*f)(Args...) const)
        : merged_hash{Archetype<Args...>().merged_hash},
          sys_ptr{system_ptr_t<S>(sys)} {
        proc_fn = [this, f](ArchetypeBlock* bl, unsigned idx) -> R {
            std::cout << "typeid(comp).name()"
                      << "\n";
            auto arguments = std::forward_as_tuple(read<Args>(bl, idx)...);
            mem_fn_t<Args...> m_fn = [this, f, arguments](Args... args) -> R {
                (sys_ptr.get()->*f)(std::forward<Args>(args)...);
            };
            std::apply(m_fn, arguments);
        };
    }
    ~SystemProcess() {}
    hash_code_t hash() override {
        std::cout << merged_hash << "\n";
        return merged_hash;
    }
    void operator()(ArchetypeBlock* bl, unsigned idx) override {
        std::cout << "typeid(comp).name()"
                  << "\n";
        proc_fn(bl, idx);
    }

   private:
    template <typename T>
    T& read(ArchetypeBlock* bl, unsigned idx) {
        using comp_t = no_ref_t<T>;
        auto& storage = bl->template get_storage<comp_t>();
        auto& comp = storage.template try_get<comp_t>(idx);
        std::cout << typeid(comp).name() << "\n";
        return comp;
    }
};  // namespace blur

}  // namespace blur