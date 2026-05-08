#include "../include/AdaptiveRadixTree.h"
#include "../include/art/Node.h"

template <typename K, typename V, typename Allocator>
AdaptiveRadixTree<K, V, Allocator>::AdaptiveRadixTree() : rootNode(nullptr) {}

template <typename K, typename V, typename Allocator>
template <typename NodeType, size_t MaxChildren>
void AdaptiveRadixTree<K, V, Allocator>::free_subtree(Node *node) {
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
AdaptiveRadixTree<K, V, Allocator>::~AdaptiveRadixTree() {
    if (!rootNode) return;
    switch (rootNode->type) {
        case NODE4: free_subtree<Node4, 4>(rootNode); break;
        case NODE16: free_subtree<Node16, 16>(rootNode); break;
        case NODE48: free_subtree<Node48, 48>(rootNode); break;
        case NODE256: free_subtree<Node256, 256>(rootNode); break;
    }
}


template <typename K, typename V, typename Allocator>
void AdaptiveRadixTree<K, V, Allocator>::insert_impl(K&& key, V&& value) {
    // insert implementation here
}

template <typename K, typename V, typename Allocator>
void AdaptiveRadixTree<K, V, Allocator>::erase_impl(K&& key) {
    // erase implementation here

}

template <typename K, typename V, typename Allocator>
Node* AdaptiveRadixTree<K, V, Allocator>::find_child(Node *node, uint8_t byte) const {
    switch (node->type) {
        case NODE4: {
            Node4 *derived4 = static_cast<Node4*>(node);
            for (size_t i = 0; i < derived4->numChildren; i++) {
                if (derived4->keys[i] == byte) {
                    return derived4->children[i];
                }
            }
            break;
        }
        case NODE16: {
            Node16 *derived16 = static_cast<Node16*>(node);
            for (size_t i = 0; i < derived16->numChildren; i++) {
                if (derived16->keys[i] == byte) {
                    return derived16->children[i];
                }
            }
            break;
        }
        case NODE48: {
            Node48 *derived48 = static_cast<Node48*>(node);
            uint8_t idx = derived48->indices[byte];
            if (idx) {
                return derived48->children[idx - 1];
            }
            break;
        }
        case NODE256: {
            Node256 *derived256 = static_cast<Node256*>(node);
            return derived256->children[byte];
            break;
        }
        case default:
            break;
    }

    return nullptr;
}

template <typename K, typename V, typename Allocator>
Node* AdaptiveRadixTree<K, V, Allocator>::search(Node *node, Encoding<K> &key, size_t depth) const {
    if (!node) return nullptr;

    if (is_leaf(node)) {
        auto leaf = get_leaf_addr<K, V>(node);
        if (leaf->key == key) {     // validate path compression
            return leaf;
        }
        return nullptr;
    }
    
    depth = depth + node->prefixLen;
    Node *next = find_child(node, key[depth]);
    return search(next, key, depth);
}

template <typename K, typename V, typename Allocator>
V* AdaptiveRadixTree<K, V, Allocator>::at_impl(K&& key) const {
    if (!rootNode) return nullptr;

    // encode key to byte array
    Encoding<K> encodedKey(std::forward<K>(key));
    Node *resultNode = search(rootNode, encodedKey, 0);

    // return value ptr if leaf found
    if (resultNode && is_leaf(resultNode)) {
        auto leaf = get_leaf_addr<K, V>(resultNode);
        return &leaf->value;
    }
    return nullptr;
}
