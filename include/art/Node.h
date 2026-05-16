#pragma once

#include <stdint.h>
#include <cstddef>

namespace ART {
    // node types (reserve lower 3 bits for type)
    static constexpr uintptr_t NODE4 = 0x01ull;
    static constexpr uintptr_t NODE16 = 0x02ull;
    static constexpr uintptr_t NODE48 = 0x03ull;
    static constexpr uintptr_t NODE256 = 0x04ull;

    static constexpr uint16_t MAX_PREFIX_LEN = UINT16_MAX;

    // should only support byte index addressable keys
    template <typename T>
    concept ARTKey = 
        std::is_trivially_copyable_v<T> &&      // trivially copyable type
        std::is_standard_layout_v<T> &&         // no virtual functions
        requires (T t, size_t i) { t[i]; } &&   // indexing
        requires (T t) { t.size(); } &&          // size() method
        requires (T t) { sizeof(T) <= ART::MAX_PREFIX_LEN; }
    ;

    // align nodes to 64 bytes for better cache performance
    // 6 lower bits for tags
    template <ARTKey K>
    struct alignas(64) Node {
        uint8_t numChildren;
        uint16_t prefixLen;
        K prefix;  // store full prefix; filled from index 0, rest is unused
    };

    // store key and child in same indices
    template <ARTKey K>
    struct Node4 {
        Node<K> header;
        uint8_t keys[4];
        Node<K> *children[4];
    };

    // same layout as Node4
    // alignas(16) for key to use SSE (128 bits) simd
    template <ARTKey K>
    struct Node16 {
        Node<K> header;
        alignas(16) uint8_t keys[16];
        Node<K> *children[16];
    };

    // directly map byte to child index
    // 1 indexed, 0 means no child
    template <ARTKey K>
    struct Node48 {
        Node<K> header;
        uint8_t indices[256];
        Node<K> *children[48];
    };

    // array of 256 children, indexed by byte
    template <ARTKey K>
    struct Node256 {
        Node<K> header;
        Node<K> *children[256];
    };

    // mask the lower 3 bits
    template <ARTKey K>
    inline uintptr_t get_type(Node<K> *node) {
        return reinterpret_cast<uintptr_t>(node) & 0B0111ull;
    }

    // strips tags
    template <ARTKey K>
    inline Node<K>* get_node(Node<K> *node) {
        return reinterpret_cast<Node<K>*>(reinterpret_cast<uintptr_t>(node) & ~0B0011'1111ull);
    }

    // strips tags and casts
    template <typename T, ARTKey K>
    requires std::is_pointer_v<T>
    inline T get_node(Node<K> *node) {
        return reinterpret_cast<T>(get_node<K>(node));
    }

    template <ARTKey K>
    inline Node<K>* tag_node(Node<K>* node, uintptr_t tag) {
        return reinterpret_cast<Node<K>*>(reinterpret_cast<uintptr_t>(node) | tag);
    }

    template <ARTKey K>
    inline bool is_full(Node<K> *node) {
        Node<K>* nodePtr = get_node(node);
        switch (get_type(node)) {
            case NODE4: return nodePtr->numChildren == 4;
            case NODE16: return nodePtr->numChildren == 16;
            case NODE48: return nodePtr->numChildren == 48;
            case NODE256: return nodePtr->numChildren == 256;
            default: throw std::runtime_error("Invalid node type");
        }
    }

    template <ARTKey K>
    inline bool is_small(Node<K> *node) {
        Node<K>* nodePtr = get_node(node);
        switch (get_type(node)) {
            case NODE4: return nodePtr->numChildren <= 1;
            case NODE16: return nodePtr->numChildren <= 3;
            case NODE48: return nodePtr->numChildren <= 12;
            case NODE256: return nodePtr->numChildren <= 32;
            default: throw std::runtime_error("Invalid node type");
        }
    }
}