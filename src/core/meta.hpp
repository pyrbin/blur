#pragma once

namespace blur {

using hash_code = size_t;

struct MetaComponentId {
    hash_code hash{0};
};

struct MetaComponentBase {
    using ConstructorFunc = void(void*);
    using DestructorFunc = void(void*);
    hash_code size{0};
    MetaComponentId id{};
    std::string name{""};
    ConstructorFunc* ctor;
    DestructorFunc* dtor;
    MetaComponentBase() {}
    MetaComponentBase(MetaComponentId id, size_t size, std::string name,
                      ConstructorFunc* ctor, DestructorFunc* dtor)
        : id{id}, size{size}, name{name}, ctor{ctor}, dtor{dtor} {}
};
template <typename C>
struct MetaComponent : public MetaComponentBase {
    constexpr MetaComponent()
        : MetaComponentBase({typeid(C).hash_code()}, sizeof(C),
                            typeid(C).name() + 1, [](void* p) { new (p) C{}; },
                            [](void* p) { ((C*)p)->~C(); }) {}
};

}