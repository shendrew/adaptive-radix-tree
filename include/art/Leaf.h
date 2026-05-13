#pragma once

#include "Node.h"
#include <type_traits>


namespace ART {
    static constexpr uintptr_t LEAF_TAG = 0x01u;

    // must reinterpret cast pointer to Node*
    // should only support byte index addressable keys
    template <typename T>
    concept ARTKey = 
        std::is_trivially_copyable_v<T> &&      // trivially copyable type
        std::is_standard_layout_v<T> &&         // no virtual functions
        requires (T t, size_t i) { t[i]; } &&   // indexing
        requires (T t) { t.size(); }            // size() method
    ;

    template <ARTKey K, typename V>
    struct alignas(64) Leaf {
        K key;
        V value;
    };

    static_assert(alignof(Node) >= 2, "Node alignment too small for ptr tagging");

    inline bool is_leaf(Node *node) {
        return (reinterpret_cast<uintptr_t>(node) & LEAF_TAG);
    }

    inline Node* make_leaf(void *value) {
        return reinterpret_cast<Node*>(reinterpret_cast<uintptr_t>(value) | LEAF_TAG);
    }

    template <ARTKey K, typename V>
    inline Leaf<K, V>* get_leaf_addr(Node *node) {
        return reinterpret_cast<Leaf<K, V>*>(reinterpret_cast<uintptr_t>(node) & ~LEAF_TAG);
    }

    template <ARTKey K, typename V>
    inline V* get_leaf_value(Node *node) {
        return &get_leaf_addr(node)->value;
    }

    template <ARTKey K, typename V>
    inline const K& get_leaf_key(Node *node) {
        return get_leaf_addr(node)->key;
    }
}
