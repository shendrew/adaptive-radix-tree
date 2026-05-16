#pragma once

#include "Node.h"
#include <type_traits>


namespace ART {
    static constexpr uintptr_t NODE_LEAF = 0x05ull;

    // must reinterpret cast pointer to Node*
    template <ARTKey K, typename V>
    struct alignas(64) Leaf {
        K key;
        V value;
    };

    template <ARTKey K, typename V>
    inline V* get_leaf_value(Node<K> *node) {
        return &(reinterpret_cast<Leaf<K, V>*>(get_node(node))->value);
    }

    template <ARTKey K, typename V>
    inline const K& get_leaf_key(Node<K> *node) {
        return reinterpret_cast<Leaf<K, V>*>(get_node(node))->key;
    }
}
