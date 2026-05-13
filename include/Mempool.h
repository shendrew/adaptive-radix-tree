#pragma once

#include <stddef.h>

namespace ART::alloc {
    // Mempool header
    class Mempool {

    public:
        void *allocate(size_t size) {
            return nullptr;
        }

        void deallocate(void *ptr, size_t size) {
            // deallocate memory
        }
    };


    inline Mempool globalPool;

    // Mempool Allocator Proxy (very low mem usage)
    template <typename T>
    class MempoolProxy {

    public:
        template <typename U>
        struct rebind {
            using other = MempoolProxy<U>;
        };

        T* allocate(std::size_t n) {
            return static_cast<T*>(globalPool.allocate(sizeof(T) * n));
        }

        void deallocate(T* ptr, std::size_t n) {
            globalPool.deallocate(ptr, sizeof(T) * n);
        }
    };
}