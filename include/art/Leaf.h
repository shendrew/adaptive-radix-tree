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

    // returns some leaf node in subtree
    inline Node* get_leaf(Node *node) {
        Node *curNode = node;
        while (!is_leaf(curNode)) {
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
        
        return curNode;
    }

    template <ARTKey K, typename V>
    inline Leaf<K, V>* get_leaf_addr(Node *node) {
        return reinterpret_cast<Leaf<K, V>*>(reinterpret_cast<uintptr_t>(node) & ~LEAF_TAG);
    }

    template <ARTKey K, typename V>
    inline V* get_leaf_value(Node *node) {
        return &get_leaf_addr<K, V>(node)->value;
    }

    template <ARTKey K, typename V>
    inline const K& get_leaf_key(Node *node) {
        return get_leaf_addr<K, V>(node)->key;
    }
}
