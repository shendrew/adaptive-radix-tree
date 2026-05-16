#include <cmath>
#include <cstring>
#include <format>
#include <iostream>

using namespace ART;

template <ARTKey K, typename V, typename Allocator>
AdaptiveRadixTree<K, V, Allocator>::AdaptiveRadixTree() : rootNode(nullptr) {}

template <ARTKey K, typename V, typename Allocator>
template <typename NodeType, size_t MaxChildren>
void AdaptiveRadixTree<K, V, Allocator>::free_subtree(Node<K> *node) {
    if (!node) return;
    NodeType *derivedNode = get_node<NodeType*>(node);

    // assume non-children are nullptr
    for (size_t i = 0; i < MaxChildren; i++) {
        Node<K> *child = derivedNode->children[i];
        if (!child) continue;
        switch (get_type(child)) {
            case NODE4: free_subtree<Node4<K>, 4>(child); break;
            case NODE16: free_subtree<Node16<K>, 16>(child); break;
            case NODE48: free_subtree<Node48<K>, 48>(child); break;
            case NODE256: free_subtree<Node256<K>, 256>(child); break;
            case NODE_LEAF: free_node<Leaf<K, V>>(get_node<Leaf<K, V>*>(child)); break;
        }
    }
    
    free_node<NodeType>(derivedNode);   // free current node
}

template <ARTKey K, typename V, typename Allocator>
AdaptiveRadixTree<K, V, Allocator>::~AdaptiveRadixTree() {
    if (!rootNode) return;
    switch (get_type(rootNode)) {
        case NODE4: free_subtree<Node4<K>, 4>(rootNode); break;
        case NODE16: free_subtree<Node16<K>, 16>(rootNode); break;
        case NODE48: free_subtree<Node48<K>, 48>(rootNode); break;
        case NODE256: free_subtree<Node256<K>, 256>(rootNode); break;
        case NODE_LEAF: free_node<Leaf<K, V>>(get_node<Leaf<K, V>*>(rootNode)); break;
    }
}


template <ARTKey K, typename V, typename Allocator>
Node<K>** AdaptiveRadixTree<K, V, Allocator>::find_child_4(Node4<K> *node, uint8_t byte) {
    for (size_t i = 0; i < node->header.numChildren; i++) {
        if (node->keys[i] == byte) {
            return &node->children[i];
        }
    }
    return nullptr;
}

template <ARTKey K, typename V, typename Allocator>
Node<K>** AdaptiveRadixTree<K, V, Allocator>::find_child_16(Node16<K> *node, uint8_t byte) {
    for (size_t i = 0; i < node->header.numChildren; i++) {
        if (node->keys[i] == byte) {
            return &node->children[i];
        }
    }
    return nullptr;
}

template <ARTKey K, typename V, typename Allocator>
Node<K>** AdaptiveRadixTree<K, V, Allocator>::find_child_48(Node48<K> *node, uint8_t byte) {
    uint8_t idx = node->indices[byte];
    if (idx) {
        return &node->children[idx - 1];
    }
    return nullptr;
}

template <ARTKey K, typename V, typename Allocator>
Node<K>** AdaptiveRadixTree<K, V, Allocator>::find_child_256(Node256<K> *node, uint8_t byte) {
    if (node->children[byte]) {
        return &node->children[byte];
    }
    return nullptr;
}

// return a double pointer so we can take a ref directly
// needs to guarantee that all NON-NULL child_ptr ==> NON-NULL *child_ptr
template <ARTKey K, typename V, typename Allocator>
inline Node<K>** AdaptiveRadixTree<K, V, Allocator>::find_child_ptr(Node<K> *node, uint8_t byte) {
    switch (get_type(node)) {
        case NODE4: return find_child_4(get_node<Node4<K>*>(node), byte);
        case NODE16: return find_child_16(get_node<Node16<K>*>(node), byte);
        case NODE48: return find_child_48(get_node<Node48<K>*>(node), byte);
        case NODE256: return find_child_256(get_node<Node256<K>*>(node), byte);
        case NODE_LEAF: throw std::runtime_error("Cannot find child of a leaf node");
        default: throw std::runtime_error("Invalid node type during child search");
    }
    return nullptr;
}

template <ARTKey K, typename V, typename Allocator>
__attribute__((noinline)) void AdaptiveRadixTree<K, V, Allocator>::grow_4(Node<K> *&node) {
    Node4<K>* derived4 = get_node<Node4<K>*>(node);
    Node<K> *newNode = tag_node(reinterpret_cast<Node<K>*>(alloc_node<Node16<K>>(Node16<K>{
        .header = {
            .numChildren = derived4->header.numChildren,
            .prefixLen = derived4->header.prefixLen,
            .prefix = derived4->header.prefix
        },
        .keys = {0},
        .children = {nullptr}
    })), NODE16);

    std::memcpy(get_node<Node16<K>*>(newNode)->keys, derived4->keys, derived4->header.numChildren * sizeof(uint8_t));
    std::memcpy(get_node<Node16<K>*>(newNode)->children, derived4->children, derived4->header.numChildren * sizeof(Node<K>*));
    
    free_node<Node4<K>>(derived4);
    node = newNode;
}

template <ARTKey K, typename V, typename Allocator>
__attribute__((noinline)) void AdaptiveRadixTree<K, V, Allocator>::grow_16(Node<K> *&node) {
    Node16<K>* derived16 = get_node<Node16<K>*>(node);
    Node<K> *newNode = tag_node(reinterpret_cast<Node<K>*>(alloc_node<Node48<K>>(Node48<K>{
        .header = {
            .numChildren = derived16->header.numChildren,
            .prefixLen = derived16->header.prefixLen,
            .prefix = derived16->header.prefix
        },
        .indices = {0},
        .children = {nullptr}
    })), NODE48);

    Node48<K> *newPtr = get_node<Node48<K>*>(newNode);
    for (size_t i = 0; i < derived16->header.numChildren; i++) {
        uint8_t byte = derived16->keys[i];
        newPtr->indices[byte] = i + 1;
        newPtr->children[i] = derived16->children[i];
    }
    free_node<Node16<K>>(derived16);
    node = newNode;
}

template <ARTKey K, typename V, typename Allocator>
__attribute__((noinline)) void AdaptiveRadixTree<K, V, Allocator>::grow_48(Node<K> *&node) {
    Node48<K>* derived48 = get_node<Node48<K>*>(node);
    Node<K> *newNode = tag_node(reinterpret_cast<Node<K>*>(alloc_node<Node256<K>>(Node256<K>{
        .header = {
            .numChildren = derived48->header.numChildren,
            .prefixLen = derived48->header.prefixLen,
            .prefix = derived48->header.prefix
        },
        .children = {nullptr}
    })), NODE256);
    Node256<K> *newPtr = get_node<Node256<K>*>(newNode);
    for (size_t byte = 0; byte < 256; byte++) {
        uint8_t idx = derived48->indices[byte];
        if (idx) {
            newPtr->children[byte] = derived48->children[idx - 1];
        }
    }
    free_node<Node48<K>>(derived48);
    node = newNode;
}

// allocate next bigger node, free old memory. fail if already size 256
template <ARTKey K, typename V, typename Allocator>
inline void AdaptiveRadixTree<K, V, Allocator>::grow(Node<K> *&node) {
    switch (get_type(node)) {
        case NODE4: {grow_4(node); return;}
        case NODE16: {grow_16(node); return;}
        case NODE48: {grow_48(node); return;}
        case NODE256: throw std::runtime_error("Cannot grow node beyond 256 children");
        case NODE_LEAF: throw std::runtime_error("Cannot grow leaf node directly");
        default: throw std::runtime_error("Invalid node type during node grow");
    }
}

template <ARTKey K, typename V, typename Allocator>
__attribute__((noinline)) void AdaptiveRadixTree<K, V, Allocator>::add_child_4(Node4<K> *parent, uint8_t byte, Node<K> *child) {    
    // find insert pos and make room
    size_t pos = 0;
    while (pos < parent->header.numChildren && parent->keys[pos] < byte) {
        pos++;
    }
    size_t shiftCnt = parent->header.numChildren - pos;
    if (shiftCnt > 0) {
        std::memmove(&parent->keys[pos+1], &parent->keys[pos], shiftCnt * sizeof(uint8_t));
        std::memmove(&parent->children[pos+1], &parent->children[pos], shiftCnt * sizeof(Node<K>*));
    }

    parent->keys[pos] = byte;
    parent->children[pos] = child;
    parent->header.numChildren++;
}

// optimize with simd later to find pos
template <ARTKey K, typename V, typename Allocator>
__attribute__((noinline)) void AdaptiveRadixTree<K, V, Allocator>::add_child_16(Node16<K> *parent, uint8_t byte, Node<K> *child) {    
    size_t pos = 0;
    while (pos < parent->header.numChildren && parent->keys[pos] < byte) {
        pos++;
    }
    size_t shiftCnt = parent->header.numChildren - pos;
    if (shiftCnt > 0) {
        std::memmove(&parent->keys[pos+1], &parent->keys[pos], shiftCnt * sizeof(uint8_t));
        std::memmove(&parent->children[pos+1], &parent->children[pos], shiftCnt * sizeof(Node<K>*));
    }

    parent->keys[pos] = byte;
    parent->children[pos] = child;
    parent->header.numChildren++;
}

template <ARTKey K, typename V, typename Allocator>
__attribute__((noinline)) void AdaptiveRadixTree<K, V, Allocator>::add_child_48(Node48<K> *parent, uint8_t byte, Node<K> *child) {
    size_t idx = 0;
    while (idx < 48 && parent->children[idx] != nullptr) {
        idx++;
    }
    parent->indices[byte] = idx + 1;
    parent->children[idx] = child;
    parent->header.numChildren++;
}

template <ARTKey K, typename V, typename Allocator>
__attribute__((noinline)) void AdaptiveRadixTree<K, V, Allocator>::add_child_256(Node256<K> *parent, uint8_t byte, Node<K> *child) {
    parent->children[byte] = child;
    parent->header.numChildren++;
}

template <ARTKey K, typename V, typename Allocator>
void AdaptiveRadixTree<K, V, Allocator>::add_child(Node<K> *&parent, uint8_t byte, Node<K> *child) {
    if (is_full(parent)) {
        grow(parent);
    }
    switch (get_type(parent)) {
        case NODE4: add_child_4(get_node<Node4<K>*>(parent), byte, child); break;
        case NODE16: add_child_16(get_node<Node16<K>*>(parent), byte, child); break;
        case NODE48: add_child_48(get_node<Node48<K>*>(parent), byte, child); break;
        case NODE256: add_child_256(get_node<Node256<K>*>(parent), byte, child); break;
        case NODE_LEAF: throw std::runtime_error("Cannot add child to a leaf node");
        default: throw std::runtime_error("Invalid node type during add child");
    }
}

// promote the single remaining child to merge with parent
template <ARTKey K, typename V, typename Allocator>
void AdaptiveRadixTree<K, V, Allocator>::shrink_4(Node<K> *&node) {
    Node4<K>* derived4 = get_node<Node4<K>*>(node);

    // find remaining chlid
    Node<K> *childPtr = nullptr;
    uint8_t branchingByte = 0;
    for (size_t i = 0; i < 4; i++) {
        if (derived4->children[i]) {
            childPtr = derived4->children[i];
            branchingByte = derived4->keys[i];
            break;
        }
    }

    // merge directly if leaf, no prefix modification
    if (get_type(childPtr) == NODE_LEAF) {
        free_node<Node4<K>>(derived4);
        node = childPtr;
        return;
    }
    
    // merge derived4 parent prefix + branching byte + child prefix
    Node<K> *childNode = get_node(childPtr);
    K mergedPrefix = derived4->header.prefix;
    reinterpret_cast<uint8_t*>(&mergedPrefix)[derived4->header.prefixLen] = branchingByte;
    std::memcpy(reinterpret_cast<uint8_t*>(&mergedPrefix) + derived4->header.prefixLen + 1,
                reinterpret_cast<uint8_t*>(&childNode->prefix),
                childNode->prefixLen);
    
    childNode->prefix = mergedPrefix;
    childNode->prefixLen = derived4->header.prefixLen + 1 + childNode->prefixLen;
    
    free_node<Node4<K>>(derived4);
    node = childPtr;
}

template <ARTKey K, typename V, typename Allocator>
void AdaptiveRadixTree<K, V, Allocator>::shrink_16(Node<K> *&node) {
    Node16<K>* derived16 = get_node<Node16<K>*>(node);
    Node<K> *newNode = tag_node(reinterpret_cast<Node<K>*>(alloc_node<Node4<K>>(Node4<K>{
        .header = {
            .numChildren = derived16->header.numChildren,
            .prefixLen = derived16->header.prefixLen,
            .prefix = derived16->header.prefix
        },
        .keys = {0},
        .children = {nullptr}
    })), NODE4);

    // copy over children
    std::memcpy(get_node<Node4<K>*>(newNode)->keys, derived16->keys, derived16->header.numChildren * sizeof(uint8_t));
    std::memcpy(get_node<Node4<K>*>(newNode)->children, derived16->children, derived16->header.numChildren * sizeof(Node<K>*));

    free_node<Node16<K>>(derived16);
    node = newNode;
}

template <ARTKey K, typename V, typename Allocator>
void AdaptiveRadixTree<K, V, Allocator>::shrink_48(Node<K> *&node) {
    Node48<K>* derived48 = get_node<Node48<K>*>(node);
    Node<K> *newNode = tag_node(reinterpret_cast<Node<K>*>(alloc_node<Node16<K>>(Node16<K>{
        .header = {
            .numChildren = derived48->header.numChildren,
            .prefixLen = derived48->header.prefixLen,
            .prefix = derived48->header.prefix
        },
        .keys = {0},
        .children = {nullptr}
    })), NODE16);

    // copy over children
    size_t idx16 = 0;
    for (size_t byte = 0; byte < 256; byte++) {
        uint8_t idx48 = derived48->indices[byte];
        if (idx48) {
            get_node<Node16<K>*>(newNode)->keys[idx16] = byte;
            get_node<Node16<K>*>(newNode)->children[idx16] = derived48->children[idx48 - 1];
            idx16++;
        }
    }

    free_node<Node48<K>>(derived48);
    node = newNode;
}

template <ARTKey K, typename V, typename Allocator>
void AdaptiveRadixTree<K, V, Allocator>::shrink_256(Node<K> *&node) {
    Node256<K>* derived256 = get_node<Node256<K>*>(node);
    Node<K> *newNode = reinterpret_cast<Node<K>*>(alloc_node<Node48<K>>(Node48<K>{
        .header = {
            .numChildren = derived256->header.numChildren,
            .prefixLen = derived256->header.prefixLen,
            .prefix = derived256->header.prefix
        },
        .indices = {0},
        .children = {nullptr}
    }));

    // copy over children
    size_t idx48 = 0;
    for (size_t byte = 0; byte < 256; byte++) {
        if (derived256->children[byte]) {
            reinterpret_cast<Node48<K>*>(newNode)->indices[byte] = idx48 + 1;
            reinterpret_cast<Node48<K>*>(newNode)->children[idx48] = derived256->children[byte];
            idx48++;
        }
    }

    free_node<Node256<K>>(derived256);
    node = tag_node(newNode, NODE48);
}

template <ARTKey K, typename V, typename Allocator>
inline void AdaptiveRadixTree<K, V, Allocator>::shrink(Node<K> *&node) {
    switch (get_type(node)) {
        case NODE4: shrink_4(node); break;
        case NODE16: shrink_16(node); break;
        case NODE48: shrink_48(node); break;
        case NODE256: shrink_256(node); break;
        case NODE_LEAF: throw std::runtime_error("Cannot shrink leaf node directly");
        default: throw std::runtime_error("Invalid node type during node shrink");
    }
}

template <ARTKey K, typename V, typename Allocator>
void AdaptiveRadixTree<K, V, Allocator>::remove_child_4(Node4<K> *parent, uint8_t byte) {
    for (size_t i = 0; i < parent->header.numChildren; i++) {
        if (parent->keys[i] == byte) {
            // shift remaining children left by 1
            size_t shiftCnt = parent->header.numChildren - i - 1;
            if (shiftCnt > 0) {
                std::memmove(&parent->keys[i], &parent->keys[i+1], shiftCnt * sizeof(uint8_t));
                std::memmove(&parent->children[i], &parent->children[i+1], shiftCnt * sizeof(Node<K>*));
            }
            
            parent->header.numChildren--;
            break;
        }
    }

    // zero out remaining child ptrs
    if (parent->header.numChildren < 4) {
        size_t shiftCnt = 4 - parent->header.numChildren;
        std::memset(&parent->keys[parent->header.numChildren], 0, shiftCnt * sizeof(uint8_t));
        std::memset(&parent->children[parent->header.numChildren], 0, shiftCnt * sizeof(Node<K>*));
    }
}

template <ARTKey K, typename V, typename Allocator>
void AdaptiveRadixTree<K, V, Allocator>::remove_child_16(Node16<K> *parent, uint8_t byte) {
    for (size_t i = 0; i < parent->header.numChildren; i++) {
        if (parent->keys[i] == byte) {
            // shift remaining children left by 1
            size_t shiftCnt = parent->header.numChildren - i - 1;
            if (shiftCnt > 0) {
                std::memmove(&parent->keys[i], &parent->keys[i+1], shiftCnt * sizeof(uint8_t));
                std::memmove(&parent->children[i], &parent->children[i+1], shiftCnt * sizeof(Node<K>*));
            }

            parent->header.numChildren--;
            break;
        }
    }

    // zero out remaining child ptrs
    if (parent->header.numChildren < 16) {
        size_t shiftCnt = 16 - parent->header.numChildren;
        std::memset(&parent->keys[parent->header.numChildren], 0, shiftCnt * sizeof(uint8_t));
        std::memset(&parent->children[parent->header.numChildren], 0, shiftCnt * sizeof(Node<K>*));
    }
}

template <ARTKey K, typename V, typename Allocator>
void AdaptiveRadixTree<K, V, Allocator>::remove_child_48(Node48<K> *parent, uint8_t byte) {
    uint8_t idx = parent->indices[byte];
    if (idx) {
        // reset values to sentinels
        parent->children[idx - 1] = nullptr;
        parent->indices[byte] = 0;
        parent->header.numChildren--;
    }
}

template <ARTKey K, typename V, typename Allocator>
void AdaptiveRadixTree<K, V, Allocator>::remove_child_256(Node256<K> *parent, uint8_t byte) {
    if (parent->children[byte]) {
        parent->children[byte] = nullptr;
        parent->header.numChildren--;
    }
}

template <ARTKey K, typename V, typename Allocator>
inline void AdaptiveRadixTree<K, V, Allocator>::remove_child(Node<K> *&parent, uint8_t byte) {
    switch (get_type(parent)) {
        case NODE4: remove_child_4(get_node<Node4<K>*>(parent), byte); break;
        case NODE16: remove_child_16(get_node<Node16<K>*>(parent), byte); break;
        case NODE48: remove_child_48(get_node<Node48<K>*>(parent), byte); break;
        case NODE256: remove_child_256(get_node<Node256<K>*>(parent), byte); break;
        case NODE_LEAF: throw std::runtime_error("Cannot remove child from a leaf node");
        default: throw std::runtime_error("Invalid node type during remove child");
    }

    if (is_small(parent)) {
        shrink(parent);
    }
}

// returns the number of matching prefix bytes
// compares key starting at depth with node prefix (0 indexed)
template <ARTKey K, typename V, typename Allocator>
inline size_t AdaptiveRadixTree<K, V, Allocator>::match_prefix(const Node<K> *nodeHeader, const K &key, size_t depth) {
    size_t matchLen = 0;
    while (matchLen < nodeHeader->prefixLen && key[depth + matchLen] == nodeHeader->prefix[matchLen]) {
        matchLen++;
    }
    return matchLen;
}

template <ARTKey K, typename V, typename Allocator>
Node<K>* AdaptiveRadixTree<K, V, Allocator>::search(Node<K> *node, K &key, size_t depth) const {
    if (!node) return nullptr;

    if (get_type(node) == NODE_LEAF) {
        auto leafKey = get_leaf_key<K, V>(node);
        if (leafKey == key) {     // validate path compression
            return node;
        }
        return nullptr;
    }
    
    auto nodePtr = get_node(node);
    size_t matchedLen = match_prefix(nodePtr, key, depth);
    if (matchedLen < nodePtr->prefixLen) {
        return nullptr;
    }
    
    depth = depth + nodePtr->prefixLen;
    Node<K> **nextPtr = find_child_ptr(node, key[depth]);
    if (!nextPtr) {
        return nullptr;
    }
    return search(*nextPtr, key, depth+1);
}


// need to take in ref to node to restructure tree if needed
template <ARTKey K, typename V, typename Allocator>
void AdaptiveRadixTree<K, V, Allocator>::insert_impl(Node<K> *&node, K &key, Node<K> *leaf, size_t depth, bool is_update) {    
    // CASE 1: empty slot, insert leaf here
    if (!node) {
        node = leaf;
        return;
    }

    // CASE 2: diverges at leaf (ie: branching byte equal, diff key)
    // split leaf into new node with 2 children
    if (get_type(node) == NODE_LEAF) {
        std::cout << "node address: " << node << std::endl;
        std::cout << "node ptr: " << get_node(node) << std::endl;


        const K& oldKey = get_leaf_key<K, V>(node);
        // check for inplace update
        if (oldKey == key) {
            if (is_update) {
                *(get_leaf_value<K, V>(node)) = *(get_leaf_value<K, V>(leaf));
            }
            free_node<Leaf<K, V>>(get_node<Leaf<K, V>*>(leaf));
            return;     // ignore if not update
        }

        // determine prefix length
        uint16_t sharedLen = 0;
        for (size_t i = depth; i < key.size() && key[i] == oldKey[i]; i++) {
            sharedLen++;
        }
        
        Node<K> *newNode = tag_node(reinterpret_cast<Node<K>*>(alloc_node<Node4<K>>(Node4<K>{
            .header = {
                .numChildren = 0,
                .prefixLen = sharedLen,
                .prefix = {}
            },
            .keys = {0},
            .children = {nullptr}
        })), NODE4);
        
        // copy shared prefix to new node
        Node4<K> *derived4 = get_node<Node4<K>*>(newNode);
        std::memcpy(reinterpret_cast<uint8_t*>(&derived4->header.prefix), 
                    reinterpret_cast<uint8_t*>(&key) + depth, 
                    sharedLen);
        
        depth = depth + sharedLen;
        add_child(newNode, key[depth], leaf);
        add_child(newNode, oldKey[depth], node);
        node = newNode;
        return;
    }

    // CASE 3: prefix split in some inner node
    // copy shared prefix to new node, update old node prefix to keep suffix
    auto nodePtr = get_node(node);
    size_t matchedPrefixLen = match_prefix(nodePtr, key, depth);
    uint8_t oldBranchingByte = nodePtr->prefix[matchedPrefixLen];
    uint8_t newBranchingByte = key[depth + matchedPrefixLen];
    if (matchedPrefixLen < nodePtr->prefixLen) {
        Node<K> *newNode = tag_node(reinterpret_cast<Node<K>*>(alloc_node<Node4<K>>(Node4<K>{
            .header = {
                .numChildren = 0,
                .prefixLen = (uint16_t)matchedPrefixLen,
                .prefix = {}
            },
            .keys = {0},
            .children = {nullptr}
        })), NODE4);
        
        // copy prefix to newNode
        Node4<K> *derived4 = get_node<Node4<K>*>(newNode);
        std::memcpy(reinterpret_cast<uint8_t*>(&derived4->header.prefix), 
                    reinterpret_cast<uint8_t*>(&nodePtr->prefix), 
                    matchedPrefixLen);

        // truncate to suffix in old node
        size_t remainingLen = nodePtr->prefixLen - matchedPrefixLen - 1;
        std::memmove(reinterpret_cast<uint8_t*>(&nodePtr->prefix), 
                     reinterpret_cast<uint8_t*>(&nodePtr->prefix) + matchedPrefixLen + 1, 
                     remainingLen);
        std::memset(reinterpret_cast<uint8_t*>(&nodePtr->prefix) + remainingLen, 0, (sizeof(K)-remainingLen) * sizeof(uint8_t));
        nodePtr->prefixLen = remainingLen;
        
        // update tree structure
        add_child(newNode, newBranchingByte, leaf);
        add_child(newNode, oldBranchingByte, node);
        node = newNode;
        return;
    }

    // CASE 4: prefixes match completely, recursively insert at child
    depth = depth + nodePtr->prefixLen;
    Node<K> **nextPtr = find_child_ptr(node, key[depth]);
    if (nextPtr) {
        insert_impl(*nextPtr, key, leaf, depth+1, is_update);
    } else {
        // CASE 5: no matching child, insert leaf here
        // guaranteed to have space since at most 256 diff byte values
        add_child(node, key[depth], leaf);
    }
}

// return true if need to erase
template <ARTKey K, typename V, typename Allocator>
bool AdaptiveRadixTree<K, V, Allocator>::erase_impl(Node<K> *&node, K &key, size_t depth) {
    if (!node) return false;

    if (get_type(node) == NODE_LEAF) {
        auto leafKey = get_leaf_key<K, V>(node);
        if (leafKey == key) {
            free_node<Leaf<K, V>>(reinterpret_cast<Leaf<K, V>*>(get_node(node)));
            return true;
        }
    }

    auto nodePtr = get_node(node);
    size_t matchedLen = match_prefix(nodePtr, key, depth);
    if (matchedLen < nodePtr->prefixLen) {
        return false;  // early exit
    }

    depth = depth + nodePtr->prefixLen;
    Node<K> **childPtr = find_child_ptr(node, key[depth]);
    if (!childPtr){
        return false;
    }
    else if (erase_impl(*childPtr, key, depth+1)) {
        remove_child(node, key[depth]);
    }

    return false;
}


template <ARTKey K, typename V, typename Allocator>
inline void AdaptiveRadixTree<K, V, Allocator>::insert(K &key, V &value) {
    Node<K> *leafNode = reinterpret_cast<Node<K>*>(alloc_node<Leaf<K, V>>(Leaf<K, V>{
        .key = key,
        .value = value
    }));
    insert_impl(rootNode, key, tag_node(leafNode, NODE_LEAF), 0, false);
}

template <ARTKey K, typename V, typename Allocator>
inline void AdaptiveRadixTree<K, V, Allocator>::update(K &key, V &value) {
    Node<K> *leafNode = reinterpret_cast<Node<K>*>(alloc_node<Leaf<K, V>>(Leaf<K, V>{
        .key = key,
        .value = value
    }));
    insert_impl(rootNode, key, tag_node(leafNode, NODE_LEAF), 0, true);
}

template <ARTKey K, typename V, typename Allocator>
inline void AdaptiveRadixTree<K, V, Allocator>::erase(K &key) {
    if (erase_impl(rootNode, key, 0)) {
        rootNode = nullptr;
    }
}

template <ARTKey K, typename V, typename Allocator>
inline V* AdaptiveRadixTree<K, V, Allocator>::at(K &key) const {
    if (!rootNode) return nullptr;

    // encode key to byte array
    Node<K> *resultNode = search(rootNode, key, 0);

    // return value ptr if leaf found
    if (resultNode) {
        return get_leaf_value<K, V>(resultNode);
    }
    return nullptr;
}

template <ARTKey K, typename V, typename Allocator>
void AdaptiveRadixTree<K, V, Allocator>::collect_stats(Node<K> *node, size_t depth, TreeStats &stats) const {
    if (!node) return;

    if (get_type(node) == NODE_LEAF) {
        stats.leaf_count++;
        stats.depths.push_back(depth);
        return;
    }

    // Count node by type
    switch (get_type(node)) {
        case NODE4: stats.node4_count++; break;
        case NODE16: stats.node16_count++; break;
        case NODE48: stats.node48_count++; break;
        case NODE256: stats.node256_count++; break;
        case NODE_LEAF: throw std::runtime_error("Should not count leaf node here");
        default: throw std::runtime_error("Invalid node type during stats collection");
    }

    // Recurse to all children
    switch (get_type(node)) {
        case NODE4: {
            Node4<K> *derived = get_node<Node4<K>*>(node);
            for (size_t i = 0; i < derived->header.numChildren; i++) {
                collect_stats(derived->children[i], depth + 1, stats);
            }
            break;
        }
        case NODE16: {
            Node16<K> *derived = get_node<Node16<K>*>(node);
            for (size_t i = 0; i < derived->header.numChildren; i++) {
                collect_stats(derived->children[i], depth + 1, stats);
            }
            break;
        }
        case NODE48: {
            Node48<K> *derived = get_node<Node48<K>*>(node);
            for (size_t i = 0; i < derived->header.numChildren; i++) {
                collect_stats(derived->children[i], depth + 1, stats);
            }
            break;
        }
        case NODE256: {
            Node256<K> *derived = get_node<Node256<K>*>(node);
            for (size_t i = 0; i < 256; i++) {
                if (derived->children[i]) {
                    collect_stats(derived->children[i], depth + 1, stats);
                }
            }
            break;
        }
    }
}

// note: written by AI
template <ARTKey K, typename V, typename Allocator>
void AdaptiveRadixTree<K, V, Allocator>::print_info() const {
    if (!rootNode) {
        std::cout << std::format("Tree is empty\n");
        return;
    }

    TreeStats stats;
    collect_stats(rootNode, 0, stats);

    size_t total_nodes = stats.node4_count + stats.node16_count + stats.node48_count + stats.node256_count;

    std::cout << std::format("\n==== Adaptive Radix Tree Info ====\n");
    std::cout << std::format("Node counts:\n");
    std::cout << std::format("  Leaf nodes: {}\n", stats.leaf_count);
    std::cout << std::format("  NODE4:  {}\n", stats.node4_count);
    std::cout << std::format("  NODE16: {}\n", stats.node16_count);
    std::cout << std::format("  NODE48: {}\n", stats.node48_count);
    std::cout << std::format("  NODE256: {}\n", stats.node256_count);
    std::cout << std::format("  Total internal nodes: {}\n", total_nodes);

    if (!stats.depths.empty()) {
        // Calculate mean
        double sum = 0;
        for (size_t d : stats.depths) {
            sum += d;
        }
        double mean = sum / stats.depths.size();

        // Calculate standard deviation
        double variance = 0;
        for (size_t d : stats.depths) {
            variance += (d - mean) * (d - mean);
        }
        variance /= stats.depths.size();
        double stddev = std::sqrt(variance);

        std::cout << std::format("\nLeaf depth statistics:\n");
        std::cout << std::format("  Mean depth: {}\n", mean);
        std::cout << std::format("  Std deviation: {}\n", stddev);
    }
    std::cout << std::format("==================================\n\n");
}
