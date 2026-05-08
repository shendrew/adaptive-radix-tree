#include "../include/art/Node.h"

inline bool is_leaf(Node *node) {
    return (reinterpret_cast<uintptr_t>(node) & LEAF_TAG);
}

inline Node* make_leaf(void *value) {
    return reinterpret_cast<Node*>(reinterpret_cast<uintptr_t>(value) | LEAF_TAG);
}

template <typename K, typename V>
inline Leaf<K, V>* get_leaf_addr(Node *node) {
    return reinterpret_cast<Leaf<K, V>*>(reinterpret_cast<uintptr_t>(node) & ~LEAF_TAG);
}

