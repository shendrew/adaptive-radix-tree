#pragma once
#define ADAPTIVE_RADIX_TREE_H

#include <memory>
#include <new>
#include <utility>
#include <vector>
#include "art/Node.h"
#include "art/Leaf.h"

namespace ART {

    struct TreeStats {
        size_t node4_count = 0;
        size_t node16_count = 0;
        size_t node48_count = 0;
        size_t node256_count = 0;
        size_t leaf_count = 0;
        std::vector<size_t> depths;
    };

    using DefaultAllocator = std::allocator<uint8_t>;

    // Adaptive Radix Tree header
    template <ARTKey K, typename V, typename Allocator = DefaultAllocator>     // default to standard single byte allocator
    class AdaptiveRadixTree{
    private:
        Allocator allocator;
        Node<K> *rootNode;

        template <typename NodeType>
        using NodeAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<NodeType>;

        template <typename NodeType, typename T>
        NodeType* alloc_node(T&& value) {
            if constexpr (std::is_same_v<Allocator, DefaultAllocator>) {
                void* ptr = operator new(sizeof(NodeType), std::align_val_t(64));
                return new (ptr) NodeType(std::forward<T>(value));
            } else {
                NodeAllocator<NodeType> allocProxy = allocator;
                NodeType* ptr = allocProxy.allocate(1);
                return new (ptr) NodeType(std::forward<T>(value));
            }
        }

        template <typename NodeType>
        void free_node(NodeType *node) {
            if constexpr (std::is_same_v<Allocator, DefaultAllocator>) {
                std::destroy_at(node);
                operator delete(node, std::align_val_t(64));
            } else {
                NodeAllocator<NodeType> allocProxy = allocator;
                std::destroy_at(node);
                allocProxy.deallocate(node, 1);
            }
        }

        template <typename NodeType, size_t MaxChildren>
        void free_subtree(Node<K> *node);

        // getters
        static inline Node<K>** find_child_ptr(Node<K> *node, uint8_t byte);
        static Node<K>** find_child_4(Node4<K> *node, uint8_t byte);
        static Node<K>** find_child_16(Node16<K> *node, uint8_t byte);
        static Node<K>** find_child_48(Node48<K> *node, uint8_t byte);
        static Node<K>** find_child_256(Node256<K> *node, uint8_t byte);

        static inline size_t match_prefix(const Node<K> *node, const K &key, size_t depth);

        // core modification helpers
        Node<K>* search(Node<K> *node, const K &key, size_t depth) const;
        bool erase_impl(Node<K> *&node, const K &key, size_t depth);
        template <bool is_update>
        void insert_impl(Node<K> *&node, const K &key, Node<K> *leaf, size_t depth);

        inline void grow(Node<K> *&node);
        void grow_4(Node<K> *&node);
        void grow_16(Node<K> *&node);
        void grow_48(Node<K> *&node);

        inline void shrink(Node<K> *&node);
        void shrink_4(Node<K> *&node);
        void shrink_16(Node<K> *&node);
        void shrink_48(Node<K> *&node);
        void shrink_256(Node<K> *&node);

        inline void add_child(Node<K> *&parent, uint8_t byte, Node<K> *child);
        void add_child_4(Node4<K> *parent, uint8_t byte, Node<K> *child);
        void add_child_16(Node16<K> *parent, uint8_t byte, Node<K> *child);
        void add_child_48(Node48<K> *parent, uint8_t byte, Node<K> *child);
        void add_child_256(Node256<K> *parent, uint8_t byte, Node<K> *child);

        inline void remove_child(Node<K> *&parent, uint8_t byte);
        void remove_child_4(Node4<K> *parent, uint8_t byte);
        void remove_child_16(Node16<K> *parent, uint8_t byte);
        void remove_child_48(Node48<K> *parent, uint8_t byte);
        void remove_child_256(Node256<K> *parent, uint8_t byte);

        void collect_stats(Node<K> *node, size_t depth, TreeStats &stats) const;

    public:
        // wrapper for leaf search result to hide internal nodes
        class Result {
        private:
            Leaf<K, V>* leafPtr;
        public:
            explicit Result(Leaf<K, V>* ptr) : leafPtr(ptr) {}
            explicit operator bool() const { return leafPtr != nullptr; }
            const K& key() const { return leafPtr->key; }
            V& value() const { return leafPtr->value; }
        };

        AdaptiveRadixTree();
        explicit AdaptiveRadixTree(const Allocator &alloc);
        ~AdaptiveRadixTree();

        inline void insert(const K& key, const V& value);
        inline void insert(const K& key, V&& value);

        inline void update(const K& key, const V& value);
        inline void update(const K& key, V&& value);

        inline void erase(const K& key);
        inline V* at(const K& key) const;
        inline Result front();

        //* can also do range queries but iterators are annoying to impl (left out for now)
        // options:
        // 1) iterator tracks stack and child ptrs then dfs (updates invalidate iter)
        // 2) store parent ptrs in nodes (annoying maintenance)
        // 3) chain leaf nodes together (very expensive insert/deletes)
        
        void print_info() const;
    };
}

// header implementation
#include "src/AdaptiveRadixTree.ipp"