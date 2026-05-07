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
V* AdaptiveRadixTree<K, V, Allocator>::find_impl(K&& key) const {
    // find implementation here
    return nullptr;
}