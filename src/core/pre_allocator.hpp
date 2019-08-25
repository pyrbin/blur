#include <cstdint>
#include <iterator>
#include <vector>
#include <iostream>

#include <cstdint>
#include <iterator>
#include <vector>
#include <iostream>

// https://stackoverflow.com/questions/21917529/is-it-possible-to-initialize-stdvector-over-already-allocated-memory
template <typename T>
class pre_allocator {
    private:
        T* memory_ptr;
        std::size_t memory_size;

    public:
        typedef std::size_t     size_type;
        typedef T*              pointer;
        typedef T               value_type;

        pre_allocator(T* memory_ptr, std::size_t memory_size)
            : memory_ptr(memory_ptr), memory_size(memory_size) {}
        
        pre_allocator(const pre_allocator& other) throw()
            : memory_ptr(other.memory_ptr), memory_size(other.memory_size) {};

        template<typename U>
        pre_allocator(const pre_allocator<U>& other) throw()
            : memory_ptr(other.memory_ptr), memory_size(other.memory_size) {};

        template<typename U>
        pre_allocator& operator = (const pre_allocator<U>& other) { return *this; }
        pre_allocator<T>& operator = (const pre_allocator& other) { return *this; }
        ~pre_allocator() {}


        pointer allocate(size_type n, const void* hint = 0) {
            return memory_ptr;
        }

        void deallocate(T* ptr, size_type n) {
            std::cout << "OKEY";
        }

        size_type max_size() const { return memory_size; }
};


