#pragma once

#include <stddef.h>
#include <type_traits>
#include <cstddef>
#include <memory>

namespace mempool {

    constexpr size_t DEFAULT_COUNT = 1000;
    constexpr size_t SMALL_SIZE = 256;
    constexpr size_t MEDIUM_SIZE = 1024;
    constexpr size_t LARGE_SIZE = 4096;

    // Mempool header
    class Mempool {
    private:
        class Pool {
            char *pool;
            size_t blockSize;
            size_t freeCnt;
            size_t *freeStack;
            size_t head;

        public:
            Pool(size_t size, size_t count) : blockSize(size), freeCnt(count), head(0) {
                pool = static_cast<char*>(std::aligned_alloc(64, blockSize * count));
                freeStack = new size_t[count];
                for (size_t i = 0; i < count; i++) {
                    freeStack[i] = i;
                }
            }
            ~Pool() {
                std::free(pool);
                delete[] freeStack;
            }

            void* allocate() {
                if (freeCnt == 0) throw std::bad_alloc();
                freeCnt--;
                return pool + blockSize * freeStack[head++];
            }

            void deallocate(void* ptr) {
                size_t offset = (static_cast<char*>(ptr) - pool) / blockSize;
                freeStack[--head] = offset;
                freeCnt++;
            }
        };

        Pool smallPool;
        Pool mediumPool;
        Pool largePool;

    public:
        // allocates 64 byte aligned pools
        Mempool(size_t smallCount = DEFAULT_COUNT, size_t mediumCount = DEFAULT_COUNT, size_t largeCount = DEFAULT_COUNT)
        : smallPool(SMALL_SIZE, smallCount)
        , mediumPool(MEDIUM_SIZE, mediumCount)
        , largePool(LARGE_SIZE, largeCount) {}

        ~Mempool() = default;
        Mempool(const Mempool &other) = delete;
        Mempool(const Mempool &&other) = delete;

        void* allocate(size_t size) {
            if (size <= SMALL_SIZE) return smallPool.allocate();
            else if (size <= MEDIUM_SIZE) return mediumPool.allocate();
            else if (size <= LARGE_SIZE) return largePool.allocate();
            else throw std::bad_alloc();
        }

        // trust that caller only deallocate valid ptrs, and at most once
        void deallocate(void *ptr, size_t size) {
            if (size <= SMALL_SIZE) return smallPool.deallocate(ptr);
            else if (size <= MEDIUM_SIZE) return mediumPool.deallocate(ptr);
            else if (size <= LARGE_SIZE) return largePool.deallocate(ptr);
            else throw std::exception();
        }
    };


    // Mempool Allocator Proxy (very low mem usage)
    template <typename T>
    class MempoolAllocator {
    private:
        // pool injected by caller, caller needs to manage lifetime
        Mempool *pool;

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

        explicit MempoolAllocator(Mempool *mempool) noexcept : pool(mempool) {}

        // create templated allocator for diff types
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
            return pool == other.pool;
        }
        template <typename U>
        bool operator!=(const MempoolAllocator<U>& other) const noexcept {
            return pool != other.pool;
        }
    };
}