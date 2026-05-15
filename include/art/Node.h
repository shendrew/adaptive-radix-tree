#pragma once

#include <stdint.h>
#include <cstddef>

namespace ART {
    // node types
    static constexpr uint8_t NODE4 = 1;
    static constexpr uint8_t NODE16 = 2;
    static constexpr uint8_t NODE48 = 3;
    static constexpr uint8_t NODE256 = 4;

    // Maximum prefix length (uint16_t bound)
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
    // Template parameter K is the key type
    template <ARTKey K>
    struct alignas(64) Node {
        uint8_t type;
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

    // same as Node4
    template <ARTKey K>
    struct Node16 {
        Node<K> header;
        uint8_t keys[16];
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


    
    template <ARTKey K>
    inline bool is_full(Node<K> *node) {
        switch (node->type) {
            case NODE4: return node->numChildren == 4;
            case NODE16: return node->numChildren == 16;
            case NODE48: return node->numChildren == 48;
            case NODE256: return node->numChildren == 256;
            default: return false;
        }
    }

}