#pragma once
#define ADAPTIVE_RADIX_TREE_H

#include <memory>
#include "Trie.h"
#include "art/Node.h"

namespace ART {

    namespace detail {
        inline Node** find_child_ptr(Node *node, uint8_t byte);

        template <typename K>
        inline size_t match_prefix(K &key1, K &key2, size_t depth);
    }

    // Adaptive Radix Tree header
    template <typename K, typename V, typename Allocator = std::allocator<uint8_t>>     // default to standard single byte allocator
    class AdaptiveRadixTree : public Trie<AdaptiveRadixTree<K, V, Allocator>, K, V> {
        friend class Trie<AdaptiveRadixTree, K, V>;

    private:
        template <typename NodeType>
        using NodeAllocator = typename Allocator::template rebind<NodeType>::other;

        template <typename NodeType, typename... Args>
        NodeType* alloc_node(Args... args) {
            NodeAllocator<NodeType> allocProxy;
            NodeType* ptr = allocProxy.allocate(1);
            return new (ptr) NodeType{args...};
        }

        template <typename NodeType>
        void free_node(Node *node) {
            NodeType *derivedNode = static_cast<NodeType*>(node);
            destroy_at(derivedNode);
            NodeAllocator<NodeType>().deallocate(derivedNode, 1);
        }

        template <typename NodeType, size_t MaxChildren>
        void free_subtree(Node *node);

        Node* search(Node *node, K &key, size_t depth) const;
        inline void add_child(Node *parent, uint8_t byte, Node *child);
        inline void insert(Node *&node, K &key, Node *leaf, size_t depth);

        // private members
        Node* rootNode;

    public:
        AdaptiveRadixTree();
        ~AdaptiveRadixTree();

        void insert_impl(K& key, V& value);
        void erase_impl(K& key);
        V* at_impl(K& key) const;
    };
}

// header implementation
#include "../src/AdaptiveRadixTree.ipp"