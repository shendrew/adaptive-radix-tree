#pragma once

#include "Node.h"
#include <type_traits>


namespace ART {
    static constexpr uintptr_t LEAF_TAG = 0x01u;

    // must reinterpret cast pointer to Node*
    template <ARTKey K, typename V>
    struct alignas(64) Leaf {
        K key;
        V value;
    };

    template <ARTKey K>
    inline bool is_leaf(Node<K> *node) {
        return (reinterpret_cast<uintptr_t>(node) & LEAF_TAG);
    }

    template <ARTKey K>
    inline Node<K>* make_leaf(void *value) {
        return reinterpret_cast<Node<K>*>(reinterpret_cast<uintptr_t>(value) | LEAF_TAG);
    }

    template <ARTKey K, typename V>
    inline Leaf<K, V>* get_leaf_addr(Node<K> *node) {
        return reinterpret_cast<Leaf<K, V>*>(reinterpret_cast<uintptr_t>(node) & ~LEAF_TAG);
    }

    template <ARTKey K, typename V>
    inline V* get_leaf_value(Node<K> *node) {
        return &get_leaf_addr<K, V>(node)->value;
    }

    template <ARTKey K, typename V>
    inline const K& get_leaf_key(Node<K> *node) {
        return get_leaf_addr<K, V>(node)->key;
    }
}
