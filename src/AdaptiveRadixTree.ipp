#ifndef ADAPTIVE_RADIX_TREE_H
#include "../include/AdaptiveRadixTree.h"
#endif

template <typename K, typename V, typename Allocator>
ART::AdaptiveRadixTree<K, V, Allocator>::AdaptiveRadixTree() : rootNode(nullptr) {}

template <typename K, typename V, typename Allocator>
template <typename NodeType, size_t MaxChildren>
void ART::AdaptiveRadixTree<K, V, Allocator>::free_subtree(Node *node) {
    if (!node) return;
    NodeType *derivedNode = static_cast<NodeType*>(node);

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

template <typename K, typename V, typename Allocator>
ART::AdaptiveRadixTree<K, V, Allocator>::~AdaptiveRadixTree() {
    if (!rootNode) return;
    switch (rootNode->type) {
        case NODE4: free_subtree<Node4, 4>(rootNode); break;
        case NODE16: free_subtree<Node16, 16>(rootNode); break;
        case NODE48: free_subtree<Node48, 48>(rootNode); break;
        case NODE256: free_subtree<Node256, 256>(rootNode); break;
    }
}


template <typename K, typename V, typename Allocator>
inline void ART::AdaptiveRadixTree<K, V, Allocator>::add_child(Node *parent, uint8_t byte, Node *child) {
    //!! TODO grow if needed
    
    if (!parent) return;
    switch (parent->type) {
        case NODE4: {
            Node4 *derived4 = static_cast<Node4*>(parent);
            derived4->keys[derived4->numChildren] = byte;
            derived4->children[derived4->numChildren] = child;
            break;
        }
        case NODE16: {  // optimize with SIMD in future
            Node16 *derived16 = static_cast<Node16*>(parent);
            derived16->keys[derived16->numChildren] = byte;
            derived16->children[derived16->numChildren] = child;
            break;
        }
        case NODE48: {
            Node48 *derived48 = static_cast<Node48*>(parent);
            derived48->indices[byte] = derived48->numChildren + 1;
            derived48->children[derived48->numChildren] = child;
            break;
        }
        case NODE256: {
            Node256 *derived256 = static_cast<Node256*>(parent);
            derived256->children[byte] = child;
            break;
        }
    }
    parent->numChildren++;
}

// returns the number of matching prefix bytes starting at a given depth
template <typename K, typename V, typename Allocator>
inline size_t ART::detail::match_prefix(Node *node, K &key, size_t depth) {
    // get key of some leaf in current subtree
    K leafKey;
    Node *curNode = node;
    while (!is_leaf()) {
        // get first child
        switch (curNode->type) {
            case NODE4: {
                Node4 *derived4 = static_cast<Node4*>(curNode);
                curNode = derived4->children[0];
                break;
            }
            case NODE16: {
                Node16 *derived16 = static_cast<Node16*>(curNode);
                curNode = derived16->children[0];
                break;
            }
            case NODE48: {
                Node48 *derived48 = static_cast<Node48*>(curNode);
                for (size_t i = 0; i < 48; i++) {
                    if (derived48->children[i]) {
                        curNode = derived48->children[i];
                        break;
                    }
                }
                break;
            }
            case NODE256: {
                Node256 *derived256 = static_cast<Node256*>(curNode);
                for (size_t i = 0; i < 256; i++) {
                    if (derived256->children[i]) {
                        curNode = derived256->children[i];
                        break;
                    }
                }
                break;
            }
        }
    }

    size_t matchLen = 0;
    while (matchLen < node->prefixLen && key[depth + matchLen] == leafKey[depth + matchLen]) {
        matchLen++;
    }
    return matchLen;
}


// need to take in ref to node to restructure tree if needed
template <typename K, typename V, typename Allocator>
inline void ART::AdaptiveRadixTree<K, V, Allocator>::insert(Node *&node, K &key, Node *leaf, size_t depth) {    
    if (!node) {
        node = leaf;
        return;
    }
    if (is_leaf(node)) {        // split leaf into new subtree
        Node *newNode = alloc_node<Node4>({
            .type = NODE4,
            .numChildren = 0,
            .prefixLen = 0
            .keys = {0},
            .children = {nullptr}
        });
        K oldKey = get_leaf_key(node);
        for (size_t i = depth; key[i] == oldKey[i] && i < key.size(); i++) {
            newNode->prefixLen++;
        }
        depth = depth + newNode->prefixLen;
        add_child(newNode, key[depth], leaf);
        add_child(newNode, oldKey[depth], node);
        node = newNode;
        return;
    }

    //! design choice: need to fetch leaf key at each step to verify prefix match
    //! slower write faster read
    



    // if prefixes still match, continue to find child after current prefix
    depth = depth + node->prefixLen;
    Node **nextPtr = detail::find_child_ptr(node, key[depth]);
    if (nextPtr) {
        insert(*nextPtr, key, leaf, depth+1);
    } else {
        add_child(node, key[depth], leaf);
    }
}


template <typename K, typename V, typename Allocator>
void ART::AdaptiveRadixTree<K, V, Allocator>::insert_impl(K &key, V &value) {
    Leaf *leafNode = alloc_node<Leaf<K, V>>(key, value);
    insert(rootNode, key, make_leaf(leafNode), 0);
}

template <typename K, typename V, typename Allocator>
void ART::AdaptiveRadixTree<K, V, Allocator>::erase_impl(K &key) {
    // erase implementation here

}


// needs to guarantee that all NON-NULL child_ptr ==> NON-NULL *child_ptr
inline ART::Node** ART::detail::find_child_ptr(Node *node, uint8_t byte) {
    switch (node->type) {
        case NODE4: {
            Node4 *derived4 = static_cast<Node4*>(node);
            for (size_t i = 0; i < derived4->numChildren; i++) {
                if (derived4->keys[i] == byte) {
                    return &derived4->children[i];
                }
            }
            break;
        }
        case NODE16: {
            Node16 *derived16 = static_cast<Node16*>(node);
            for (size_t i = 0; i < derived16->numChildren; i++) {
                if (derived16->keys[i] == byte) {
                    return &derived16->children[i];
                }
            }
            break;
        }
        case NODE48: {
            Node48 *derived48 = static_cast<Node48*>(node);
            uint8_t idx = derived48->indices[byte];
            if (idx) {
                return &derived48->children[idx - 1];
            }
            break;
        }
        case NODE256: {
            Node256 *derived256 = static_cast<Node256*>(node);
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

template <typename K, typename V, typename Allocator>
ART::Node* ART::AdaptiveRadixTree<K, V, Allocator>::search(Node *node, K &key, size_t depth) const {
    if (!node) return nullptr;

    if (is_leaf(node)) {
        auto leaf = get_leaf_addr<K, V>(node);
        if (leaf->key == key) {     // validate path compression
            return leaf;
        }
        return nullptr;
    }
    
    depth = depth + node->prefixLen;
    Node **nextPtr = find_child_ptr(node, key[depth]);
    if (!nextPtr) {
        return nullptr;
    }
    return search(*nextPtr, key, depth+1);
}

template <typename K, typename V, typename Allocator>
V* ART::AdaptiveRadixTree<K, V, Allocator>::at_impl(K &key) const {
    if (!rootNode) return nullptr;

    // encode key to byte array
    Node *resultNode = search(rootNode, key, 0);

    // return value ptr if leaf found
    if (resultNode && is_leaf(resultNode)) {
        auto leaf = get_leaf_addr<K, V>(resultNode);
        return &leaf->value;
    }
    return nullptr;
}
