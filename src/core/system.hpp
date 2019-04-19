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

template <typename... Cs>
class Archetype;

class Entity;
class ArchetypeBlock;

using hash_code_t = size_t;
using hash_list = std::vector<ComponentMetaBase>&;

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
    ComponentMask signature;
    virtual void operator()(ArchetypeBlock* bl, unsigned idx) = 0;
    template <typename S, typename R = void, typename... Args>
    void set_mem_fn(void (S::*f)(Args...) const) {
        static_assert(true, "Can't set system function in base class!");
    };
};

template <typename S>
struct SystemProcess : public SystemProcessBase {
   public:
    system_ptr_t<S> sys_ptr;
    proc_fn_t proc_fn;

    SystemProcess(S* sys) : sys_ptr{system_ptr_t<S>(sys)} {}
    template <typename R = void, typename... Args>
    void set_mem_fn(void (S::*f)(Args...) const) {
        signature = ComponentMask();
        signature.merge<Args...>();
        proc_fn = [this, f](ArchetypeBlock* bl, unsigned idx) -> R {
            auto arguments =
                std::forward_as_tuple(fetch_comp<Args>(bl, idx)...);
            mem_fn_t<Args...> m_fn = [this, f, arguments](Args... args) -> R {
                (sys_ptr.get()->*f)(std::forward<Args>(args)...);
            };
            std::apply(m_fn, arguments);
        };
    }
    ~SystemProcess() {}
    constexpr void operator()(ArchetypeBlock* bl, unsigned idx) override {
        return proc_fn(bl, idx);
    }

   private:
    template <typename T>
    T& fetch_comp(ArchetypeBlock* bl, unsigned idx) {
        using comp_t = no_ref_t<T>;
        auto& storage = bl->template get_storage<comp_t>();
        auto& comp = storage.template try_get<comp_t>(idx);
        return comp;
    }
};  // namespace blur

}  // namespace blur