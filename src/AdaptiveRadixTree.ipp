#ifndef ADAPTIVE_RADIX_TREE_H
#include "include/AdaptiveRadixTree.h"
#endif

using namespace ART;

template <ARTKey K, typename V, typename Allocator>
AdaptiveRadixTree<K, V, Allocator>::AdaptiveRadixTree() : rootNode(nullptr) {}

template <ARTKey K, typename V, typename Allocator>
template <typename NodeType, size_t MaxChildren>
void AdaptiveRadixTree<K, V, Allocator>::free_subtree(Node *node) {
    if (!node) return;
    NodeType *derivedNode = reinterpret_cast<NodeType*>(node);

    // assume non-children are nullptr
    for (size_t i = 0; i < MaxChildren; i++) {
        Node *child = derivedNode->children[i];
        if (!child) continue;
        switch (child->type) {
            case NODE4: free_subtree<Node4, 4>(child); break;
            case NODE16: free_subtree<Node16, 16>(child); break;
            case NODE48: free_subtree<Node48, 48>(child); break;
            case NODE256: free_subtree<Node256, 256>(child); break;
        }
    }
    
    free_node<NodeType>(node);   // free current node
}

template <ARTKey K, typename V, typename Allocator>
AdaptiveRadixTree<K, V, Allocator>::~AdaptiveRadixTree() {
    if (!rootNode) return;
    switch (rootNode->type) {
        case NODE4: free_subtree<Node4, 4>(rootNode); break;
        case NODE16: free_subtree<Node16, 16>(rootNode); break;
        case NODE48: free_subtree<Node48, 48>(rootNode); break;
        case NODE256: free_subtree<Node256, 256>(rootNode); break;
    }
}


// allocate next bigger node, free old memory. fail if already size 256
template <ARTKey K, typename V, typename Allocator>
inline void AdaptiveRadixTree<K, V, Allocator>::grow(Node *&node) {
    Node *newNode = nullptr;
    switch (node->type) {
        case NODE4: {
            Node4 *derived4 = reinterpret_cast<Node4*>(node);
            newNode = reinterpret_cast<Node*>(alloc_node<Node16>(Node16{
                .header = {
                    .type = NODE16,
                    .numChildren = derived4->header.numChildren,
                    .prefixLen = derived4->header.prefixLen
                },
                .keys = {0},
                .children = {nullptr}
            }));
            Node16 *newPtr = reinterpret_cast<Node16*>(newNode);
            for (size_t i = 0; i < derived4->header.numChildren; i++) {
                newPtr->keys[i] = derived4->keys[i];
                newPtr->children[i] = derived4->children[i];
            }
            free_node<Node4>(node);
            break;
        }
        case NODE16: {
            Node16 *derived16 = reinterpret_cast<Node16*>(node);
            newNode = reinterpret_cast<Node*>(alloc_node<Node48>(Node48{
                .header = {
                    .type = NODE48,
                    .numChildren = derived16->header.numChildren,
                    .prefixLen = derived16->header.prefixLen
                },
                .indices = {0},
                .children = {nullptr}
            }));
            Node48 *newPtr = reinterpret_cast<Node48*>(newNode);
            for (size_t i = 0; i < derived16->header.numChildren; i++) {
                uint8_t byte = derived16->keys[i];
                newPtr->indices[byte] = i + 1;
                newPtr->children[i] = derived16->children[i];
            }
            free_node<Node16>(node);
            break;
        }
        case NODE48: {
            Node48 *derived48 = reinterpret_cast<Node48*>(node);
            newNode = reinterpret_cast<Node*>(alloc_node<Node256>(Node256{
                .header = {
                    .type = NODE256,
                    .numChildren = derived48->header.numChildren,
                    .prefixLen = derived48->header.prefixLen
                },
                .children = {nullptr}
            }));
            Node256 *newPtr = reinterpret_cast<Node256*>(newNode);
            for (size_t i = 0; i < 256; i++) {
                uint8_t idx = derived48->indices[i];
                if (idx) {
                    newPtr->children[i] = derived48->children[idx - 1];
                }
            }
            free_node<Node48>(node);
            break;
        }
        case NODE256:
            throw std::runtime_error("Cannot grow node beyond 256 children");
    }
    node = newNode;
}

template <ARTKey K, typename V, typename Allocator>
inline void AdaptiveRadixTree<K, V, Allocator>::add_child(Node *&parent, uint8_t byte, Node *child) {
    if (is_full(parent)) {
        grow(parent);
    }
    
    if (!parent) return;
    switch (parent->type) {
        case NODE4: {
            Node4 *derived4 = reinterpret_cast<Node4*>(parent);
            derived4->keys[derived4->header.numChildren] = byte;
            derived4->children[derived4->header.numChildren] = child;
            break;
        }
        case NODE16: {  // optimize with SIMD in future
            Node16 *derived16 = reinterpret_cast<Node16*>(parent);
            derived16->keys[derived16->header.numChildren] = byte;
            derived16->children[derived16->header.numChildren] = child;
            break;
        }
        case NODE48: {
            Node48 *derived48 = reinterpret_cast<Node48*>(parent);
            derived48->indices[byte] = derived48->header.numChildren + 1;
            derived48->children[derived48->header.numChildren] = child;
            break;
        }
        case NODE256: {
            Node256 *derived256 = reinterpret_cast<Node256*>(parent);
            derived256->children[byte] = child;
            break;
        }
    }
    parent->numChildren++;
}

// returns the number of matching prefix bytes starting at a given depth
template <ARTKey K>
inline size_t detail::match_prefix(const K &key1, const K &key2, size_t depth, size_t prefixLen) {
    size_t matchLen = 0;
    while (matchLen < prefixLen && key1[depth + matchLen] == key2[depth + matchLen]) {
        matchLen++;
    }
    return matchLen;
}


// need to take in ref to node to restructure tree if needed
template <ARTKey K, typename V, typename Allocator>
inline void AdaptiveRadixTree<K, V, Allocator>::insert(Node *&node, K &key, Node *leaf, size_t depth) {    
    if (!node) {
        node = leaf;
        return;
    }
    if (is_leaf(node)) {        // split leaf into new subtree
        Node *newNode = reinterpret_cast<Node*>(alloc_node<Node4>(Node4{
            .header = {
                .type = NODE4,
                .numChildren = 0,
                .prefixLen = 0
            },
            .keys = {0},
            .children = {nullptr}
        }));
        const K& oldKey = get_leaf_key<K, V>(node);
        for (size_t i = depth; key[i] == oldKey[i] && i < key.size(); i++) {
            newNode->prefixLen++;
        }
        depth = depth + newNode->prefixLen;
        add_child(newNode, key[depth], leaf);
        add_child(newNode, oldKey[depth], node);
        node = newNode;
        return;
    }

    // split current node if new key branches in prefix
    // design choice: need to fetch leaf key at each step to verify path compression
    // => slower write faster read
    Node *leafInSubtree = get_leaf(node);
    const K& oldKey = get_leaf_key<K, V>(leafInSubtree);
    size_t matchedPrefixLen = detail::match_prefix(key, oldKey, depth, node->prefixLen);
    if (matchedPrefixLen < node->prefixLen) {
        Node *newNode = reinterpret_cast<Node*>(alloc_node<Node4>(Node4{
            .header = {
                .type = NODE4,
                .numChildren = 0,
                .prefixLen = (uint16_t)matchedPrefixLen
            },
            .keys = {0},
            .children = {nullptr}
        }));

        node->prefixLen = node->prefixLen - (matchedPrefixLen + 1);   // adjust old prefix len
        add_child(newNode, key[depth + matchedPrefixLen], leaf);
        add_child(newNode, oldKey[depth + matchedPrefixLen], node);
        node = newNode;
        return;
    }

    // if prefixes still match, continue to find child after current prefix
    depth = depth + node->prefixLen;
    Node **nextPtr = detail::find_child_ptr(node, key[depth]);
    if (nextPtr) {
        insert(*nextPtr, key, leaf, depth+1);
    } else {
        add_child(node, key[depth], leaf);
    }
}


template <ARTKey K, typename V, typename Allocator>
void AdaptiveRadixTree<K, V, Allocator>::insert_impl(K &key, V &value) {
    auto *leafNode = alloc_node<Leaf<K, V>>(Leaf<K, V>{
        .key = key,
        .value = value
    });
    insert(rootNode, key, make_leaf(leafNode), 0);
}

template <ARTKey K, typename V, typename Allocator>
void AdaptiveRadixTree<K, V, Allocator>::erase_impl(K &key) {
    // erase implementation here

}


// needs to guarantee that all NON-NULL child_ptr ==> NON-NULL *child_ptr
inline Node** detail::find_child_ptr(Node *node, uint8_t byte) {
    switch (node->type) {
        case NODE4: {
            Node4 *derived4 = reinterpret_cast<Node4*>(node);
            for (size_t i = 0; i < derived4->header.numChildren; i++) {
                if (derived4->keys[i] == byte) {
                    return &derived4->children[i];
                }
            }
            break;
        }
        case NODE16: {
            Node16 *derived16 = reinterpret_cast<Node16*>(node);
            for (size_t i = 0; i < derived16->header.numChildren; i++) {
                if (derived16->keys[i] == byte) {
                    return &derived16->children[i];
                }
            }
            break;
        }
        case NODE48: {
            Node48 *derived48 = reinterpret_cast<Node48*>(node);
            uint8_t idx = derived48->indices[byte];
            if (idx) {
                return &derived48->children[idx - 1];
            }
            break;
        }
        case NODE256: {
            Node256 *derived256 = reinterpret_cast<Node256*>(node);
            if (derived256->children[byte]) {
                return &derived256->children[byte];
            }
            return nullptr;
        }
        default:
            break;
    }

    return nullptr;
}

template <ARTKey K, typename V, typename Allocator>
Node* AdaptiveRadixTree<K, V, Allocator>::search(Node *node, K &key, size_t depth) const {
    if (!node) return nullptr;

    if (is_leaf(node)) {
        auto leafKey = get_leaf_key<K, V>(node);
        if (leafKey == key) {     // validate path compression
            return node;
        }
        return nullptr;
    }
    
    depth = depth + node->prefixLen;
    Node **nextPtr = detail::find_child_ptr(node, key[depth]);
    if (!nextPtr) {
        return nullptr;
    }
    return search(*nextPtr, key, depth+1);
}

template <ARTKey K, typename V, typename Allocator>
V* AdaptiveRadixTree<K, V, Allocator>::at_impl(K &key) const {
    if (!rootNode) return nullptr;

    // encode key to byte array
    Node *resultNode = search(rootNode, key, 0);

    // return value ptr if leaf found
    if (resultNode) {
        return get_leaf_value<K, V>(resultNode);
    }
    return nullptr;
}
