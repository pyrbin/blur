#pragma once

#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>

namespace blur {

class Entity;
class ArchetypeBlock;

struct SystemFunctorBase {
   public:
    ~SystemFunctorBase(){};
    template <typename S, typename R = void, typename... Args>
    void init(void (S::*f)(Args...) const) {
        static_assert(true, "Dont init base system fun");
    }
    virtual bool valid_mask(ComponentMask) const = 0;
    virtual void operator()(unsigned idx, ArchetypeBlock*) const = 0;

   protected:
    template <typename System>
    using system_ptr_t = std::unique_ptr<System>;
    using fn_ptr_t = std::function<void(unsigned idx, ArchetypeBlock*)>;
    template <typename... Args>
    using mod_ptr_t = std::function<void(Args...)>;
};
template <typename S>
struct SystemFunctor : public SystemFunctorBase {
   public:
    SystemFunctor(S* s) : sysptr{system_ptr_t<S>(s)} {}
    template <typename R = void, typename... Args>
    void init(void (S::*f)(Args...) const) {
        comp_mask = ComponentMask::of<Args...>();
        fn = [this, f](unsigned idx, ArchetypeBlock* ab) -> void {
            auto modfn = [this, f](Args... args) -> void {
                (sysptr.get()->*f)(std::forward<Args>(args)...);
            };
            ab->modify_components(idx, static_cast<mod_ptr_t<Args...>>(modfn));
        };
    }

    constexpr bool valid_mask(ComponentMask other) const override {
        return other.contains(comp_mask);
    }

    void operator()(unsigned idx, ArchetypeBlock* ab) const override {
        fn(idx, ab);
    }

   private:
    ComponentMask comp_mask{0};
    system_ptr_t<S> sysptr;
    fn_ptr_t fn;
};

}  // namespace blur