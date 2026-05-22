#pragma once

#include <stddef.h>
#include <type_traits>
#include <cstddef>
#include <memory>

namespace mempool {

    constexpr size_t BLOCK_SIZE = 4096;

    // Mempool header
    class Mempool {

    public:
        void *allocate(size_t size) {
            return nullptr;
        }

        void deallocate(void *ptr, size_t size) {
            // deallocate memory
        }

        Mempool* get() {
            return this;
        }
    };


    // Mempool Allocator Proxy (very low mem usage)
    template <typename T>
    class MempoolAllocator {
    private:
        std::shared_ptr<Mempool> pool;

        template <typename U>
        friend class MempoolAllocator;

    public:
        // AI generated: Standard allocator type definitions
        using value_type = T;
        using pointer = T*;
        using const_pointer = const T*;
        using reference = T&;
        using const_reference = const T&;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using is_always_equal = std::true_type;

        // allow alloc to bind to any type
        template <typename U>
        struct rebind {
            using other = MempoolAllocator<U>;
        };

        MempoolAllocator() : pool(std::make_shared<Mempool>()) {}

        // create templated allocator for diff types
        // copy shared_ptr to new template instance U
        // !!! currently not thread safe
        template <typename U>
        MempoolAllocator(const MempoolAllocator<U>& other) noexcept : pool(other.pool) {}

        T* allocate(std::size_t n) {
            return static_cast<T*>(pool->allocate(sizeof(T) * n));
        }

        void deallocate(T* ptr, std::size_t n) {
            pool->deallocate(ptr, sizeof(T) * n);
        }

        // AI generated these, i don't currently use them
        // might be needed if trees get copied and want to share pool
        template <typename U>
        bool operator==(const MempoolAllocator<U>& other) const noexcept {
            return pool.get() == other.pool.get();
        }
        template <typename U>
        bool operator!=(const MempoolAllocator<U>& other) const noexcept {
            return pool.get() != other.pool.get();
        }
    };
}