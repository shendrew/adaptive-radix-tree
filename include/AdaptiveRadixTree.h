#pragma once
#define ADAPTIVE_RADIX_TREE_H

#include <memory>
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

    // Adaptive Radix Tree header
    template <ARTKey K, typename V, typename Allocator = std::allocator<uint8_t>>     // default to standard single byte allocator
    class AdaptiveRadixTree{
    private:
        template <typename NodeType>
        using NodeAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<NodeType>;

        template <typename NodeType, typename T>
        NodeType* alloc_node(T&& value) {
            NodeAllocator<NodeType> allocProxy;
            NodeType* ptr = allocProxy.allocate(1);
            return new (ptr) NodeType(std::forward<T>(value));
        }

        template <typename NodeType>
        void free_node(NodeType *node) {
            std::destroy_at(node);
            NodeAllocator<NodeType>().deallocate(node, 1);
        }

        template <typename NodeType, size_t MaxChildren>
        void free_subtree(Node<K> *node);

        // getters
        static inline Node<K>** find_child_ptr(Node<K> *node, uint8_t byte);
        static Node<K>** find_child_4(Node4<K> *node, uint8_t byte);
        static Node<K>** find_child_16(Node16<K> *node, uint8_t byte);
        static Node<K>** find_child_48(Node48<K> *node, uint8_t byte);
        static Node<K>** find_child_256(Node256<K> *node, uint8_t byte);

        static inline size_t match_prefix(Node<K> *node, const K &key, size_t depth);

        // core modification helpers
        Node<K>* search(Node<K> *node, K &key, size_t depth) const;
        void insert_impl(Node<K> *&node, K &key, Node<K> *leaf, size_t depth, bool is_update);
        bool erase_impl(Node<K> *&node, K &key, size_t depth);

        inline void grow(Node<K> *&node);
        void grow_4(Node<K> *&node);
        void grow_16(Node<K> *&node);
        void grow_48(Node<K> *&node);

        void shrink_4(Node<K> *&node);
        void shrink_16(Node<K> *&node);
        void shrink_48(Node<K> *&node);
        void shrink_256(Node<K> *&node);

        inline void add_child(Node<K> *&parent, uint8_t byte, Node<K> *child);
        void add_child_4(Node<K> *&parent, uint8_t byte, Node<K> *child);
        void add_child_16(Node<K> *&parent, uint8_t byte, Node<K> *child);
        void add_child_48(Node<K> *&parent, uint8_t byte, Node<K> *child);
        void add_child_256(Node<K> *&parent, uint8_t byte, Node<K> *child);

        inline void remove_child(Node<K> *&parent, uint8_t byte);
        void remove_child_4(Node<K> *&parent, uint8_t byte);
        void remove_child_16(Node<K> *&parent, uint8_t byte);
        void remove_child_48(Node<K> *&parent, uint8_t byte);
        void remove_child_256(Node<K> *&parent, uint8_t byte);

        void collect_stats(Node<K> *node, size_t depth, TreeStats &stats) const;

        // private members
        Node<K>* rootNode;

    public:
        AdaptiveRadixTree();
        ~AdaptiveRadixTree();

        inline void insert(K& key, V& value);
        inline void update(K& key, V& value);
        inline void erase(K& key);
        inline V* at(K& key) const;
        
        void print_info() const;
    };
}

// header implementation
#include "src/AdaptiveRadixTree.ipp"