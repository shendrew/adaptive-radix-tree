#pragma once

#include <stdint.h>
#include <cstddef>
#include <type_traits>

// node types
constexpr uint8_t NODE4 = 1;
constexpr uint8_t NODE16 = 2;
constexpr uint8_t NODE48 = 3;
constexpr uint8_t NODE256 = 4;

// pointer tags
constexpr uintptr_t LEAF_TAG = 0x01u;

// align nodes to 16 bytes for better cache performance
struct alignas(64) Node {
    uint8_t type;
    uint8_t numChildren;
    uint16_t prefixLen;
};

// store key and child in same indices
struct Node4 : public Node {
    uint8_t keys[4];
    Node *children[4];
};

// same as Node4
struct Node16 : public Node {
    uint8_t keys[16];
    Node *children[16];
};

// directly map byte to child index
// 1 indexed, 0 means no child
struct Node48 : public Node {
    uint8_t indices[256];
    Node *children[48];
};

// array of 256 children, indexed by byte
struct Node256 : public Node {
    Node *children[256];
};

// must reinterpret cast pointer to Node*
// should only support byte index addressable keys
template <typename T>
concept ARTKey = 
    std::is_trivially_copyable_v<T> &&      // trivially copyable type
    std::is_standard_layout_v<T> &&         // no virtual functions
    requires (T t, size_t i) { t[i]; }      // indexing
;

template <ARTKey K, typename V>
struct alignas(64) Leaf {
    K key;
    V value;
};


// helper functions
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
