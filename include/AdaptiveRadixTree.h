#pragma once
#define ADAPTIVE_RADIX_TREE_H

#include <memory>
#include <utility>
#include <vector>
#include "Trie.h"
#include "art/Node.h"
#include "art/Leaf.h"

namespace ART {

    namespace detail {
        inline Node** find_child_ptr(Node *node, uint8_t byte);
        Node** find_child_4(Node4 *node, uint8_t byte);
        Node** find_child_16(Node16 *node, uint8_t byte);
        Node** find_child_48(Node48 *node, uint8_t byte);
        Node** find_child_256(Node256 *node, uint8_t byte);

        template <ARTKey K>
        inline size_t match_prefix(const K &key1, const K &key2, size_t depth, size_t prefixLen);
    }

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
    class AdaptiveRadixTree : public Trie<AdaptiveRadixTree<K, V, Allocator>, K, V> {
        friend class Trie<AdaptiveRadixTree, K, V>;

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
        void free_node(Node *node) {
            NodeType *derivedNode = reinterpret_cast<NodeType*>(node);
            std::destroy_at(derivedNode);
            NodeAllocator<NodeType>().deallocate(derivedNode, 1);
        }

        template <typename NodeType, size_t MaxChildren>
        void free_subtree(Node *node);

        Node* search(Node *node, K &key, size_t depth) const;
        void insert(Node *&node, K &key, Node *leaf, size_t depth, bool is_update);

        inline void grow(Node *&node);
        void grow_4(Node4 *&node);
        void grow_16(Node16 *&node);
        void grow_48(Node48 *&node);
        
        void add_child(Node *&parent, uint8_t byte, Node *child);

        void collect_stats(Node *node, size_t depth, TreeStats &stats) const;

        // private members
        Node* rootNode;

    public:
        AdaptiveRadixTree();
        ~AdaptiveRadixTree();

        inline void insert_impl(K& key, V& value);
        inline void update_impl(K& key, V& value);
        inline void erase_impl(K& key);
        inline V* at_impl(K& key) const;
        
        void print_info() const;
    };
}

// header implementation
#include "src/AdaptiveRadixTree.ipp"