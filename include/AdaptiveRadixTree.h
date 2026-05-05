#pragma once

#include <memory>
#include "Trie.h"

// Adaptive Radix Tree header
template <typename Allocator = std::allocator<uint8_t>>     // default to standard single byte allocator
class AdaptiveRadixTree : public Trie<AdaptiveRadixTree> {
    friend class Trie<AdaptiveRadixTree>;

private:
    template <typename NodeType>
    using NodeAllocator = typename Allocator::template rebind<NodeType>::other;

    template <typename NodeType>
    NodeType* alloc_node() {
        NodeAllocator<NodeType> allocProxy;
        NodeType* ptr = allocProxy.allocate(1);
        return new (ptr) NodeType();
    }

private:
    template <typename K, typename V>
    void insert_impl(K&& key, V&& value);

    template <typename K>
    void erase_impl(K&& key);

    template <typename K>
    void* find_impl(K&& key) const;
};