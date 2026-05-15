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

    // Template version for is_leaf
    template <ARTKey K>
    inline bool is_leaf(Node<K> *node) {
        return (reinterpret_cast<uintptr_t>(node) & LEAF_TAG);
    }

    template <ARTKey K>
    inline Node<K>* make_leaf(void *value) {
        return reinterpret_cast<Node<K>*>(reinterpret_cast<uintptr_t>(value) | LEAF_TAG);
    }

    // returns some leaf node in subtree
    template <ARTKey K>
    inline Node<K>* get_leaf(Node<K> *node) {
        Node<K> *curNode = node;
        while (!is_leaf(curNode)) {
            // get first child
            switch (curNode->type) {
                case NODE4: {
                    Node4<K> *derived4 = reinterpret_cast<Node4<K>*>(curNode);
                    curNode = derived4->children[0];
                    break;
                }
                case NODE16: {
                    Node16<K> *derived16 = reinterpret_cast<Node16<K>*>(curNode);
                    curNode = derived16->children[0];
                    break;
                }
                case NODE48: {
                    Node48<K> *derived48 = reinterpret_cast<Node48<K>*>(curNode);
                    for (size_t i = 0; i < 48; i++) {
                        if (derived48->children[i]) {
                            curNode = derived48->children[i];
                            break;
                        }
                    }
                    break;
                }
                case NODE256: {
                    Node256<K> *derived256 = reinterpret_cast<Node256<K>*>(curNode);
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
        
        return curNode;
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
